#!/usr/bin/python
# from automation import *
import srtm
import os
from os import walk
import sys
import subprocess
import shutil
import binascii
import tempfile
from tqdm import tqdm
import csv
import json
import random
from random import randint
import sys
import subprocess
import numpy as np
import matplotlib.pyplot as plt
import psutil
import time
import math
import signal
from os import listdir
from os.path import isfile, join
import thread
from math import cos, sin, asin, sqrt, radians, floor
import myGrass
import pandas as pd
from sklearn.cluster import DBSCAN
import Queue
import string
import random
from shapely.geometry import MultiPoint
from geopy.distance import great_circle
import copy
import time
import glob
import math

class coordinate:
    def __init__(self, lat, lng, demand):
        self.lat = float(lat)
        self.lng = float(lng)
        self.demand = int(demand)
        self.revenue = 0
        self.index = 0


class mapBoundary:
    def __init__(self):
        self.tRight = coordinate(0,0,0)
        self.tLeft = coordinate(0,0,0)
        self.bRight = coordinate(0,0,0)
        self.bLeft = coordinate(0,0,0)

class metaData:
    def __init__(self, cols, rows, distX, distY, nodata, elevationData):
        self.cols = int(cols)
        self.rows = int(rows)
        self.distX = int(distX)
        self.distY = int(distY)
        self.nodata = int(nodata)
        self.elevationData = int(elevationData)
        self.indexMapping = []
    def getUnitXAndY(self):
        unitX = (self.distX / self.cols) * 1000
        unitY = (self.distY / self.rows) * 1000
        return unitX, unitY

class clusterPack:
    def __init__(self):
        self.centerCoordinate = coordinate(0,0,0)
        self.centerCoordinateIndex = -1
        self.clusterPackCoordinates = []
        self.clusterDirectory = ""
        self.clusterId = "-1"
        self.mapBoundary = mapBoundary()
        self.epsilon = 0
        self.isParent = False
        self.needToRunFCNF = True
        self.scaling = -1
        self.needFurtherClustering = False
    def calculateClusterScaling(self, dx, dy, bounds):
        horizontalDist = math.ceil(calculateDistance(bounds.tLeft, bounds.tRight) * 1000)
        verticalDist = math.ceil(calculateDistance(bounds.tLeft, bounds.bLeft) * 1000)
        xScale = math.ceil(horizontalDist / dx)
        yScale = math.ceil(verticalDist / dy)
        self.scaling = xScale * yScale



def incrementPointByXAndY(refCoordinate, dx, dy):
    newCoordinate = coordinate(0,0,0)
    newCoordinate.lat = refCoordinate.lat + (180 / math.pi) * (dy / 6371)
    newCoordinate.lng = refCoordinate.lng + (180 / math.pi) * (dx / 6371) / cos(refCoordinate.lat * (math.pi / 180) )
    return newCoordinate;

def calculateDistance(coordinate1, coordinate2):
    lon1, lat1, lon2, lat2 = map(radians, [coordinate1.lng, coordinate1.lat, coordinate2.lng, coordinate2.lat])
    dlon = lon2 - lon1
    dlat = lat2 - lat1
    a = sin(dlat / 2) ** 2 + cos(lat1) * cos(lat2) * sin(dlon / 2) ** 2
    c = 2 * asin(sqrt(a))
    km = 6371 * c
    return km

def calculateMapBoundries(inputCoordinates):
    minx = coordinate(sys.maxint,0, -1)
    miny = coordinate(0, sys.maxint, -1)
    maxx = coordinate(-sys.maxint-1, 0, -1)
    maxy = coordinate(0, -sys.maxint-1, -1)
    for coord in inputCoordinates:
        if coord.lat < minx.lat:
            minx = coord
        if coord.lng < miny.lng:
            miny = coord
        if coord.lat > maxx.lat:
            maxx = coord
        if coord.lng > maxy.lng:
            maxy = coord
    boundingSquare = mapBoundary()
    boundingSquare.tLeft.lat = maxx.lat
    boundingSquare.tLeft.lng = miny.lng
    boundingSquare.tRight.lat = maxx.lat
    boundingSquare.tRight.lng = maxy.lng
    boundingSquare.bLeft.lat = minx.lat
    boundingSquare.bLeft.lng = miny.lng
    boundingSquare.bRight.lat = minx.lat
    boundingSquare.bRight.lng = maxy.lng

    deltaX = calculateDistance(boundingSquare.tLeft, boundingSquare.tRight) * 0.10
    if deltaX == 0.0:
        deltaX = 0.1
    deltaY = calculateDistance(boundingSquare.tLeft, boundingSquare.bLeft) * 0.10
    if deltaY == 0.0:
        deltaY = 0.1
    newPointToTopLeft = incrementPointByXAndY(boundingSquare.tLeft, -deltaX, deltaY)
    newPointToBottomRight = incrementPointByXAndY(boundingSquare.bRight, deltaX, -deltaY)
    boundingSquare.tLeft = newPointToTopLeft
    boundingSquare.bRight = newPointToBottomRight
    boundingSquare.tRight.lat = newPointToTopLeft.lat
    boundingSquare.tRight.lng = newPointToBottomRight.lng
    boundingSquare.bLeft.lat = newPointToBottomRight.lat
    boundingSquare.bLeft.lng = newPointToTopLeft.lng

    # print "Map Boundaries:"
    # print str(boundingSquare.tRight.lat)+","+str(boundingSquare.tRight.lng)
    # print str(boundingSquare.tLeft.lat)+","+str(boundingSquare.tLeft.lng)
    # print str(boundingSquare.bRight.lat)+","+str(boundingSquare.bRight.lng)
    # print str(boundingSquare.bLeft.lat)+","+str(boundingSquare.bLeft.lng)
    return boundingSquare


def downloadMapTiles(customerLocations, mapBoundaries):
    filenames = []
    filenamesTemp = {}
    elevation_data = srtm.get_data(srtm3=True, srtm1=False)
    print "Downloading map tiles"

    for i in range(0, int(abs(math.floor(mapBoundaries.bRight.lng) - math.floor(mapBoundaries.bLeft.lng))) + 1):
        elevation_data.get_elevation(mapBoundaries.bLeft.lat, mapBoundaries.bLeft.lng + i)
        filenamesTemp[elevation_data.get_file_name(mapBoundaries.bLeft.lat, mapBoundaries.bLeft.lng + i)] = 1
        # filenamesShapeTemp[elevation_data.get_swbd_file_name(mapBoundaries.bLeft.lat, mapBoundaries.bLeft.lng + i)] = 1
        for j in range(0, int(abs(math.floor(mapBoundaries.tLeft.lat) - math.floor(mapBoundaries.bLeft.lat)))+1):
            elevation_data.get_elevation(mapBoundaries.bLeft.lat + j, mapBoundaries.bLeft.lng + i)
            filenamesTemp[elevation_data.get_file_name(mapBoundaries.bLeft.lat + j, mapBoundaries.bLeft.lng + i)] = 1
            # filenamesShapeTemp[elevation_data.get_swbd_file_name(mapBoundaries.bLeft.lat + j, mapBoundaries.bLeft.lng + i)] = 1

    # fixme: check if BLeft < Tleft
    # for i in range(0, int(math.ceil(abs(mapBoundaries.tRight.lng - mapBoundaries.tLeft.lng)))+1):
    #     for j in range(0, int(math.ceil(abs(mapBoundaries.tLeft.lat - mapBoundaries.bLeft.lat)))+1):
    #         # print mapBoundaries.tLeft.lng + i, mapBoundaries.bLeft.lat + j
    #         elevation_data.get_elevation(mapBoundaries.bLeft.lat + j, mapBoundaries.tLeft.lng + i)
    #         filenamesTemp[elevation_data.get_file_name(mapBoundaries.bLeft.lat + j, mapBoundaries.tLeft.lng + i)] = 1

    # elevation_data.get_elevation(mapBoundaries.tRight.lat, mapBoundaries.tRight.lng)
    # filenamesTemp[elevation_data.get_file_name(mapBoundaries.bRight.lat, mapBoundaries.bRight.lng)] = 1
    # elevation_data.get_elevation(mapBoundaries.tLeft.lat,  mapBoundaries.tLeft.lng)
    # filenamesTemp[(elevation_data.get_file_name(mapBoundaries.tLeft.lat,  mapBoundaries.tLeft.lng))] = 1
    # elevation_data.get_elevation(mapBoundaries.bLeft.lat,  mapBoundaries.bLeft.lng)
    # filenamesTemp[(elevation_data.get_file_name(mapBoundaries.bLeft.lat,  mapBoundaries.bLeft.lng))] = 1
    # elevation_data.get_elevation(mapBoundaries.bRight.lat, mapBoundaries.bRight.lng)
    # filenamesTemp[(elevation_data.get_file_name(mapBoundaries.bRight.lat, mapBoundaries.bRight.lng))] = 1

    # for coords in customerLocations:
    #     elevation_data.get_elevation(coords.lat, coords.lng)
    #     filenamesTemp[(elevation_data.get_file_name(coords.lat, coords.lng))] = 1

    for file in filenamesTemp:
        if file is not None:
            filenames.append(file.split(".")[0])

    return filenames

def createGlobalMap(gscript, outputDir, fileNames, customerLocationsFile, mapBoundaries, scaling, mapMetaData):

    print "Importing srtm files"
    for file in tqdm(fileNames):
        if file is not None:
            # gscript.run_command('r.in.srtm', flags="1", quiet=QUIET, overwrite=True, input=os.path.join(tempDirForMapTiles,file))
            gscript.run_command('r.in.srtm',  quiet=QUIET, flags="1", overwrite=True, input=os.path.join(tempDirForMapTiles, file))
    gscript.run_command("g.region", flags='s', raster=fileNames)

    print "Patching files"
    if len(fileNames) > 1:
        gscript.run_command('r.patch', quiet=QUIET, input=fileNames, output='patched')
    else:
        gscript.run_command('g.copy', quiet=QUIET, raster=fileNames[0] + ',patched')
    # remove nulls from the map
    gscript.run_command('r.null', map='patched', null='-9999')

    north = mapBoundaries.tRight.lat
    south = mapBoundaries.bRight.lat
    east = mapBoundaries.tRight.lng
    west = mapBoundaries.bLeft.lng

    print north, south, east, west
    print "Applying boundaries"
    gscript.run_command('g.region', flags='a', n=north, s=south, e=east, w=west)

    xDim = calculateDistance(mapBoundaries.tLeft, mapBoundaries.tRight)
    yDim = calculateDistance(mapBoundaries.tLeft, mapBoundaries.bLeft)
    mapMetaData.distX = xDim
    mapMetaData.distY = yDim
    print str(xDim)+", "+str(yDim)

    delta = math.sqrt((xDim * yDim)/scaling)
    resRows = math.sqrt(scaling) #math.ceil(yDim / delta)
    resCols = math.sqrt(scaling) #math.ceil(xDim / delta)
    print resRows
    print resCols

    print "Resampling"
    gscript.run_command('g.region', flags='a', rows=resRows, cols=resCols)

    if customerLocationsFile != "":
        gscript.run_command('v.in.ascii', input=customerLocationsFile, output='sinks', separator='comma', x=2, y=1)

    gscript.run_command('r.resample', quiet=QUIET, input='patched', output='patched_resampled')

    print "Creating index mapping"
    gscript.run_command('r.to.vect', quiet=QUIET, input='patched_resampled', output='index_mapping', type='point')

    print "Exporting index mapping to", outputDir+'/index_mapping.txt'
    gscript.run_command('v.out.ascii', quiet=QUIET, overwrite=True, input='index_mapping', output=outputDir+'/index_mapping.txt', format='point')

    print "Removing elevation < 0m from the map"
    gscript.mapcalc('patched_resampled = if(patched_resampled > 0, patched_resampled, null())', overwrite=True)

    print "Exporting the resampled map to", outputDir+'/out.asc'
    gscript.run_command('r.out.gdal', quiet=QUIET, overwrite=True, input='patched_resampled', output=outputDir+'/out.asc', format='AAIGrid')

    return outputDir+'/out.asc'

def getMetaData(mapFile, mapMetaData, outputDir):

    with open(outputDir + "/out.asc", 'r') as e:
        mapMetaData.cols = int(e.readline().strip().split()[1])
        mapMetaData.rows = int(e.readline().strip().split()[1])
        e.readline()
        e.readline()
        e.readline()
        e.readline()
        line = e.readline().strip()
        matrix = []
        mapMetaData.nodata = 65535
        if line.strip().split(" ")[0] == "NODATA_value":
            mapMetaData.nodata = int(line.split(" ")[1])
        else:
            matrix.append(map(int, line.strip().split(" ")))
        for line in e:
            matrix.append(map(int, line.strip().split(" ")))
        mapMetaData.elevationData = matrix #mistake (previously: data.matrix (matrix is not the member of class metaData)) confirmation required
    with open(outputDir + '/index_mapping.txt', 'r') as indexMappingFile:
        for mapping in indexMappingFile:
            coordsWithIndex = mapping.split('|')
            mapCoord = coordinate(float(coordsWithIndex[1]), float(coordsWithIndex[0]), -1)
            mapCoord.index = int(coordsWithIndex[2])
            mapMetaData.indexMapping.append(mapCoord)
    indexMappingFile.close()
    return mapMetaData

def calculateViewsheds(mapFile, mapMetaData, towerHeight, outputDir):

    print 'Starting CUDA'
    print "thp: double check the metaData"
    startcmd = './algorithms/CUDA/main '+mapFile + " " + str(mapMetaData.rows * mapMetaData.cols) + " " + str(towerHeight) + " " +outputDir+"/out_"+str(mapMetaData.rows * mapMetaData.cols)+".bin"
    print startcmd
    p = subprocess.Popen(startcmd, shell=True,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    if p.returncode != 0:
        print >>sys.stderr, 'ERROR: %s' % mapMetaData.rows * mapMetaData.cols
        sys.exit(-1)
    else:
        print 'CUDA finished'

def getNearestIndex(indexMappingDict, fromPoint):
    minDistance = sys.float_info.max
    nearestPoint = -1
    for idx, mappedPoint in enumerate(indexMappingDict):
        distance = calculateDistance(fromPoint, mappedPoint)
        if distance <= minDistance:
            minDistance = distance
            nearestPoint = idx
    return nearestPoint

def getNearestPoint(indexMappingDict, fromPoint):
    minDistance = sys.float_info.max
    nearestPoint = None
    nearestPointIndex = -1
    for idx, mappedPoint in enumerate(indexMappingDict):
        distance = calculateDistance(fromPoint, mappedPoint)
        if distance <= minDistance:
            minDistance = distance
            nearestPointIndex = idx
            nearestPoint = mappedPoint
    return nearestPoint, nearestPointIndex

def truncateLastNewLineAndClose(finalHandel):
    finalHandel.seek(-1, os.SEEK_END)
    finalHandel.truncate()
    finalHandel.close()

def checkForTheClosestOne(sink, i, j, mapMetaData, oldNearestPoint):
    minDistance = sys.float_info.max
    oneRight = oneLeft = topRight = topLeft = i * mapMetaData.cols + (j)
    if (j+1) < mapMetaData.cols:
        oneRight = i * mapMetaData.cols + (j+1)
    if (j-1) >= 0:
        oneLeft = i * mapMetaData.cols + (j-1)
    oneTop = oneBottom = bottomRight = bottomLeft = (i) * mapMetaData.cols + j
    if (i-1) >= 0:
        oneTop = (i-1) * mapMetaData.cols + j
    if (i+1) < mapMetaData.rows:
        oneBottom = (i+1) * mapMetaData.cols + j
    if (i-1) >= 0 and (j+1) < mapMetaData.cols:
        topRight = (i-1) * mapMetaData.cols + (j+1)
    if (i+1) < mapMetaData.rows and (j+1) < mapMetaData.cols:
        bottomRight = (i+1) * mapMetaData.cols + (j+1)
    if (i-1) >= 0 and (j-1) >= 0:
        topLeft = (i-1) * mapMetaData.cols + (j-1)
    if (i+1) < mapMetaData.rows  and (j-1) >= 0:
        bottomLeft = (i+1) * mapMetaData.cols + (j-1)
    distanceFromRight = calculateDistance(sink, mapMetaData.indexMapping[oneRight])
    distanceFromLeft = calculateDistance(sink, mapMetaData.indexMapping[oneLeft])
    distanceFromTop = calculateDistance(sink, mapMetaData.indexMapping[oneTop])
    distanceFromBottom = calculateDistance(sink, mapMetaData.indexMapping[oneBottom])
    distanceFromTopRight = calculateDistance(sink, mapMetaData.indexMapping[topRight])
    distanceFromBottomRight = calculateDistance(sink, mapMetaData.indexMapping[bottomRight])
    distanceFromTopLeft = calculateDistance(sink, mapMetaData.indexMapping[topLeft])
    distanceFromBottomLeft = calculateDistance(sink, mapMetaData.indexMapping[bottomLeft])
    finalNearestPointIndex = oldNearestPoint
    if (j+1) < mapMetaData.cols and mapMetaData.elevationData[i][j+1] != mapMetaData.nodata and distanceFromRight < minDistance:
        minDistance = distanceFromRight
        finalNearestPointIndex = oneRight
    if (j-1) >= 0 and mapMetaData.elevationData[i][j-1] != mapMetaData.nodata and distanceFromLeft < minDistance:
        minDistance = distanceFromLeft
        finalNearestPointIndex = oneLeft
    if (i-1) >= 0 and mapMetaData.elevationData[i-1][j] != mapMetaData.nodata and distanceFromTop < minDistance:
        minDistance = distanceFromTop
        finalNearestPointIndex = oneTop
    if (i+1) < mapMetaData.rows and mapMetaData.elevationData[i+1][j] != mapMetaData.nodata and distanceFromBottom < minDistance:
        minDistance = distanceFromBottom
        finalNearestPointIndex = oneBottom
    ###check for the diagonals
    if (i-1) >= 0 and (j+1) < mapMetaData.cols and mapMetaData.elevationData[i-1][j+1] != mapMetaData.nodata and distanceFromTopRight < minDistance:
        minDistance = distanceFromTopRight
        finalNearestPointIndex = topRight
    if (i+1) < mapMetaData.rows and (j+1) < mapMetaData.cols and mapMetaData.elevationData[i+1][j+1] != mapMetaData.nodata and distanceFromBottomRight < minDistance:
        minDistance = distanceFromBottomRight
        finalNearestPointIndex = bottomRight
    if (i-1) >= 0 and (j-1) >= 0 and mapMetaData.elevationData[i-1][j-1] != mapMetaData.nodata and distanceFromTopLeft < minDistance:
        minDistance = distanceFromTopLeft
        finalNearestPointIndex = topLeft
    if (i+1) < mapMetaData.rows  and (j-1) >= 0 and mapMetaData.elevationData[i+1][j-1] != mapMetaData.nodata and distanceFromBottomLeft < minDistance:
        minDistance = distanceFromBottomLeft
        finalNearestPointIndex = bottomLeft
    return finalNearestPointIndex + 1

def assignUniqueSource(mapMetaData, sourceIndex, inputSinks):
    sourceSinkSame = False
    for sink in inputSinks:
        if sourceIndex == int(sink["node"])-1:
            sourceSinkSame = True
    newSourceIndex = sourceIndex
    potentialSources = []
    if sourceSinkSame:
        minDistance = sys.float_info.max
        for mapping in mapMetaData.indexMapping:
            isPresentInSinks = False
            for sink in inputSinks:
                if int(sink["node"]) == mapping.index:
                    isPresentInSinks = True
            if not isPresentInSinks:
                potentialSources.append(mapping)
        for p_source in potentialSources:
            distance = calculateDistance(p_source, mapMetaData.indexMapping[sourceIndex])
            if distance <= minDistance:
                newSourceIndex = p_source.index
                minDistance = distance
        newSourceIndex -= 1
    return newSourceIndex



def createInputFileForAlgorithm(mapMetaData, outputDir, inputCoordinates, sourceCoordinates):
    jsonDATA = []
    totalSource = 0
    nearestPointIndex = 0
    for sink in inputCoordinates:
        minDistance = sys.float_info.max
        for idx, mapCoord in enumerate(mapMetaData.indexMapping):
            PI = (idx) / mapMetaData.cols
            PJ = (idx) % mapMetaData.cols
            distance = calculateDistance(sink, mapCoord)
            if distance <= minDistance:
                minDistance = distance
                nearestPointIndex = idx + 1
                if mapMetaData.elevationData[PI][PJ] == mapMetaData.nodata:
                    nearestPointIndex = checkForTheClosestOne(sink, PI, PJ, mapMetaData, nearestPointIndex - 1)
        nodeProperty = {"type": "sink", "capacity": sink.demand, "mountingHeight": 1, "frequencies": [2.4, 3, 5, 24],
                        "revenue": sink.revenue}  # randint(1000, 5500)
        coordinates = {"lat": mapMetaData.indexMapping[nearestPointIndex - 1].lat,
                       "lng": mapMetaData.indexMapping[nearestPointIndex - 1].lng}
        totalSource += sink.demand
        jsonDATA.append({"node": int(nearestPointIndex), "nodeProperty": nodeProperty, "coordinates": coordinates})
    nodeProperty = {"type": "source", "capacity": totalSource, "mountingHeight": 1, "frequencies": [2.4, 3, 5, 24]}
    nearestSourceIndex = getNearestIndex(mapMetaData.indexMapping, sourceCoordinates)
    nearestSourceIndex = assignUniqueSource(mapMetaData, nearestSourceIndex, jsonDATA)
    sourceCoordinates = {"lat": sourceCoordinates.lat, "lng": sourceCoordinates.lng}
    jsonDATA.append({"node": nearestSourceIndex + 1, "nodeProperty": nodeProperty, "coordinates": sourceCoordinates})
    with open(outputDir + '/input.json', "wb") as inputJson:
        inputJson.write(json.dumps({"nodes": jsonDATA}, ensure_ascii=False))
    inputJson.close()

def createPinnedJSON(mapMetaData, pinnedEdgesFile, outputDir):
    outputPinnedJsonFileHandel = open(outputDir+"/pinned_edges.json", "w")
    jsondata = []
    try:
        with open(pinnedEdgesFile) as  pinnedEdgesFileHandel:
            for pinnedEdge in pinnedEdgesFileHandel:
                edgeProperty = {"bandwidth":int(pinnedEdge.rstrip().split(",")[4])}
                tailPt = coordinate(float(pinnedEdge.rstrip().split(",")[0]), float(pinnedEdge.rstrip().split(",")[1]), 0)
                headPt = coordinate(float(pinnedEdge.rstrip().split(",")[2]), float(pinnedEdge.rstrip().split(",")[3]), 0)
                tailOrigPt, tailIndex = getNearestPoint(mapMetaData.indexMapping, tailPt)
                headOrigPt, headIndex = getNearestPoint(mapMetaData.indexMapping, headPt)
                jsondata.append({"edgeProperty":edgeProperty,"nodes":[tailIndex, headIndex]})
        pinnedEdgesFileHandel.close()
    except:
        print "No Pinning Input Exist"
    outputPinnedJsonFileHandel.write(json.dumps({"edges":jsondata}, ensure_ascii=False) )
    outputPinnedJsonFileHandel.close()

def createTowerPinnedJson(mapMetaData, towerPinnedFile, outputDir):
    outputPinnedJsonFileHandel = open(outputDir + "/pinned_tower.json", "w")
    jsondata = []
    try:
        with open(towerPinnedFile) as pinnedTowerFileHandel:
            for pinnedTower in pinnedTowerFileHandel:
                towerCoordinate = coordinate(float(pinnedTower.rstrip().split(",")[0]), float(pinnedTower.rstrip().split(",")[1]), 0)
                towerOrigPt, towerIndex = getNearestPoint(mapMetaData.indexMapping, towerCoordinate)
                jsondata.append(towerIndex)
        pinnedTowerFileHandel.close()
    except:
        print "No pinning tower input exist"
    outputPinnedJsonFileHandel.write(json.dumps({"pinnedTowers":jsondata}, ensure_ascii=False))
    outputPinnedJsonFileHandel.close()

def get_centermost_point(cluster):
    centroid = (MultiPoint(cluster).centroid.x, MultiPoint(cluster).centroid.y)
    centermost_point = min(cluster, key=lambda point: great_circle(point, centroid).m)
    return len(cluster),tuple(centermost_point)

def removeDuplicates(customerLocationFile):
    column_names = ['lat','lon', 'demand', 'revenue']
    df = pd.read_csv(customerLocationFile, header = None, names = column_names)

    coords = df.as_matrix(columns=['lat', 'lon'])
    kms_per_radian = 6371.0088
    customers = 9999
    e = 0.05
    #while (customers > 200):
    epsilon = float(e) / kms_per_radian
    db = DBSCAN(eps=epsilon, min_samples=1, algorithm='ball_tree', metric='haversine').fit(np.radians(coords))
    cluster_labels = db.labels_
    num_clusters = len(set(cluster_labels))
    clusters = pd.Series([coords[cluster_labels == n] for n in range(num_clusters)])
    # print('Number of clusters: {}'.format(num_clusters))

    demands = []
    revenues = []
    for cluster in clusters:
        demand = 0
        revenue = 0
        for c in cluster:
            demand = demand + df.loc[(df['lat'] == c[0]) & (df['lon'] == c[1]), 'demand'].values[0]
            revenue = revenue + df.loc[(df['lat'] == c[0]) & (df['lon'] == c[1]), 'revenue'].values[0]
        demands.append(demand)
        revenues.append(revenue)

    centermost_points = clusters.map(get_centermost_point)
    count, coord = zip(*centermost_points)
    lats, lons = zip(*coord)
    rep_points = pd.DataFrame({'lon':lons, 'lat':lats, 'cnt':count, 'demands':demands, 'revenue':revenues})

    coordinates = []
    for i in range(num_clusters):
        uniqueChild = coordinate(rep_points['lat'].values[i], rep_points['lon'].values[i], rep_points['demands'].values[i])
        uniqueChild.revenue = rep_points['revenue'].values[i]
        coordinates.append(uniqueChild)
    uniqueSinkFile = open(customerLocationFile[:-4]+"_unique.csv", "w")
    for cord in coordinates:
        uniqueSinkFile.write(str(cord.lat)+","+str(cord.lng)+","+str(cord.demand)+","+str(cord.revenue)+"\n")
    truncateLastNewLineAndClose(uniqueSinkFile)
    return customerLocationFile[:-4]+"_unique.csv", coordinates

def removeAllFiles(directory):
    fileList = [f for f in os.listdir(directory)]
    for f in fileList:
        os.remove(os.path.join(directory, f))

def makeClusters(customerLocationFile, outputDir, tmpDir, epsilon, oldId, mapMetaData):
    clusterDir = outputDir+"/clusters"
    try:
        os.mkdir(clusterDir)
    except:
        removeAllFiles(clusterDir)
        print "Clustering Dir Exist"
    cmdClustering = './algorithms/clustering/clustering.R ' + customerLocationFile + ' ' + str(epsilon) + ' ' + clusterDir
    print cmdClustering
    p1 = subprocess.Popen(cmdClustering, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    outClustering, err = p1.communicate()
    if p1.returncode != 0:
        print >> sys.stderr, 'ERROR: %s' % err
        sys.exit(-1)
    fileList = [f for f in os.listdir(clusterDir)]
    print len(fileList)
    allClusterPacks = []
    for clusterNum in range(1,len(fileList)+1,1):
        pack = clusterPack()
        pack.epsilon = int(outClustering.split(" ")[1])
        pack.clusterId = str(oldId) + "_" + str(clusterNum)
        pack.clusterDirectory = outputDir + "/" + str(pack.clusterId)
        totalDemandOfCluster = 0
        totalRevenueOfCluster = 0
        clusterTempFile = open(tmpDir+"/clusterTemp.txt", "w")
        for point in open(clusterDir+"/"+str(clusterNum)):
            lat = point.rstrip().split(",")[1]
            lng = point.rstrip().split(",")[0]
            totalDemandOfCluster += int(point.rstrip().split(",")[2])
            totalRevenueOfCluster += int(point.rstrip().split(",")[3])
            childCoordinate = coordinate(lat, lng, int(point.rstrip().split(",")[2]))
            childCoordinate.revenue = int(point.rstrip().split(",")[3])
            pack.clusterPackCoordinates.append(childCoordinate)
            np, npi = getNearestPoint(mapMetaData.indexMapping, childCoordinate)
            clusterTempFile.write(str(npi)+"\n")
        truncateLastNewLineAndClose(clusterTempFile)
            # calculate map boundaries for each cluster
        boundingBox = calculateMapBoundries(pack.clusterPackCoordinates)
        if boundingBox.tRight.lat == boundingBox.tLeft.lat == boundingBox.bRight.lat == boundingBox.bLeft.lat:  # means only one point in a cluster
            pack.centerCoordinate = boundingBox.tRight
            pack.centerCoordinateIndex = getNearestIndex(mapMetaData.indexMapping, pack.centerCoordinate)
            pack.centerCoordinate.demand = totalDemandOfCluster
            pack.centerCoordinate.revenue = totalRevenueOfCluster
        else:
            topRightNearestPoint, topRightNearestPointIndex = getNearestPoint(mapMetaData.indexMapping,boundingBox.tRight)
            topLeftNearestPoint, topLeftNearestPointIndex = getNearestPoint(mapMetaData.indexMapping, boundingBox.tLeft)
            bottomRightNearestPoint, bottomRightNearestPointIndex = getNearestPoint(mapMetaData.indexMapping,boundingBox.bRight)
            bottomLeftNearestPoint, bottomLeftNearestPointIndex = getNearestPoint(mapMetaData.indexMapping,boundingBox.bLeft)
            print "Start read binary file..."
            cmdReadBinary = './algorithms/readBinary/readBinary.o ' + tmpDir + ' ' + str(topRightNearestPointIndex) + " " + str(topLeftNearestPointIndex) + " " + str(bottomRightNearestPointIndex) + " " + str(bottomLeftNearestPointIndex) + " 10"
            print cmdReadBinary
            p2 = subprocess.Popen(cmdReadBinary, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            out, err = p2.communicate()
            if p2.returncode != 0:
                print >> sys.stderr, 'ERROR: %s' % err
                sys.exit(-1)
            print "CenterPoint: ", pack.clusterId, out
            centerPointIndex = out.rstrip().split(",")[0]
            pack.centerCoordinate = copy.deepcopy(mapMetaData.indexMapping[int(centerPointIndex)])
            pack.centerCoordinate.demand = copy.deepcopy(totalDemandOfCluster)
            pack.centerCoordinate.revenue = copy.deepcopy(totalRevenueOfCluster)
            pack.centerCoordinateIndex = int(centerPointIndex)
            if out.rstrip().split(",")[1] == "FALSE":
                pack.needToRunFCNF = False
            if pack.centerCoordinate.lng > boundingBox.tRight.lng:
                print "topRightNearestPointIndex extend"
                boundingBox.tRight.lng = max(topRightNearestPoint.lng, boundingBox.tRight.lng)
                boundingBox.bRight.lng = max(bottomRightNearestPoint.lng, boundingBox.bRight.lng)
            if pack.centerCoordinate.lng < boundingBox.tLeft.lng:
                boundingBox.tLeft.lng = min(topLeftNearestPoint.lng, boundingBox.tLeft.lng)
                boundingBox.bLeft.lng = min(topLeftNearestPoint.lng, boundingBox.tLeft.lng)
                print "bottomRightNearestPointIndex extend"
            if pack.centerCoordinate.lat > boundingBox.tLeft.lat:
                boundingBox.tLeft.lat = max(topLeftNearestPoint.lat, boundingBox.tLeft.lat)
                boundingBox.tRight.lat = max(topRightNearestPoint.lat, boundingBox.tRight.lat)
                print "topLeftNearestPointIndex extend"
            if pack.centerCoordinate.lat <  boundingBox.bLeft.lat:
                boundingBox.bLeft.lat = min(bottomLeftNearestPoint.lat, boundingBox.bLeft.lat)
                boundingBox.bRight.lat = min(bottomRightNearestPoint.lat, boundingBox.bRight.lat)
                print "bottomLeftNearestPointIndex extend"
        pack.mapBoundary = boundingBox
        dx, dy = mapMetaData.getUnitXAndY()
        pack.calculateClusterScaling(dx,dy, boundingBox)
        allClusterPacks.append(pack)
        os.remove(tmpDir+"/clusterTemp.txt")
    return int(outClustering.split(" ")[1]), allClusterPacks



def readClusters(clusterDir, mapMetaData, secondLayer, clusterIdP):
    listOfClusterFiles = glob.glob(clusterDir+"/*")
    allClusterPacks = []
    for file in listOfClusterFiles:
        if not os.path.isfile(file):
            continue
        else:
            pack = clusterPack()
            clusterId = clusterIdP + "_" + file.split("/")[len(file.split("/"))-1][:-4].split("_")[1]
            if clusterId == "_1_0":
                a = 2+2
                print "chuss"
            totalDemandOfCluster = 0
            totalRevenueOfCluster = 0
            with open(file) as clusterFileHandel:
                lineInd = 0
                for line in clusterFileHandel:
                    if lineInd == 0:
                        centerCoordinateLine = line.rstrip().split(" ")[1]
                    else:
                        totalDemandOfCluster += 5
                        totalRevenueOfCluster += 1500
                        childCoordinate = coordinate(float(line.rstrip().split(",")[1]), float(line.rstrip().split(",")[0]), 5)
                        childCoordinate.revenue = 1500
                        childCoordinate.index = int(line.rstrip().split(",")[2])
                        pack.clusterPackCoordinates.append(childCoordinate)
                    lineInd+=1
                pack.centerCoordinate = coordinate(float(centerCoordinateLine.split(",")[1]), float(centerCoordinateLine.split(",")[0]), totalDemandOfCluster)
                pack.centerCoordinate.revenue = totalRevenueOfCluster
                pack.centerCoordinateIndex = getNearestIndex(mapMetaData.indexMapping, pack.centerCoordinate)
                pack.centerCoordinate.index = pack.centerCoordinateIndex
                boundingBox = calculateMapBoundries(pack.clusterPackCoordinates)
                pack.mapBoundary = boundingBox
                dx, dy = mapMetaData.getUnitXAndY()
                pack.calculateClusterScaling(dx, dy, boundingBox)
                pack.clusterId = clusterId
                if secondLayer:
                    pack.needFurtherClustering = True
                allClusterPacks.append(pack)
            clusterFileHandel.close()
    return allClusterPacks



def isNeedToRunFCNF(outputDir, source, equipmentsFile, mapMetaData, isNeedToRunVisibilityChecker):
    if not isNeedToRunVisibilityChecker:
        with open(outputDir+"/output_center.json", "w") as outputJsonFileHandel:
            outputJsonFileHandel.write(json.dumps({"center":[source.lat,source.lng]}))
        outputJsonFileHandel.close()
        return False
    cmdVisibilityChecker = './algorithms/visibilityChecker/visibilityChecker ' + str(outputDir) + ' ' + equipmentsFile + ' ' + str(source.demand) + ' 10'
    print cmdVisibilityChecker
    p = subprocess.Popen(cmdVisibilityChecker, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    if p.returncode != 0:
        print >> sys.stderr, 'ERROR: %s' % err
    if out == "FALSE":
        heights = [15, 20, 25, 30]
        for h in heights:
            calculateViewsheds(outputDir + "/out.asc", mapMetaData, h, outputDir)
            cmdVisibilityChecker = './algorithms/visibilityChecker/visibilityChecker ' + str(outputDir) + ' ' + equipmentsFile + ' ' + str(source.demand) + ' ' + str(h)
            p = subprocess.Popen(cmdVisibilityChecker, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            out, err = p.communicate()
            print "Check visibility for: ", h, "and the result is", out
            if p.returncode != 0:
                print >> sys.stderr, 'ERROR: %s' % err
            if out != "FALSE":
                generateGPSCoordOutput(outputDir + "/output.json", outputDir + "/output_coords.txt", mapMetaData)
                print "Importing resulting graph into GRASS"
                gscript.run_command('v.in.ascii', quiet=QUIET, overwrite=True,
                                    input=outputDir + "/output_coords.txt",
                                    output="OmniLink", format="standard", flags="n")
                return False
        calculateViewsheds(outputDir + "/out.asc", mapMetaData, 10, outputDir)
        return True
    else:
        generateGPSCoordOutput(outputDir+"/output.json", outputDir+"/output_coords.txt", mapMetaData)
        print "Importing resulting graph into GRASS"
        gscript.run_command('v.in.ascii', quiet=QUIET, overwrite=True,
                            input=outputDir+"/output_coords.txt",
                            output="OmniLink", format="standard", flags="n")
    return False




def isSourceCanSeeAllSinks(sinks, source, outputDir, mapMetaData):
    for sink in sinks:
        distance = calculateDistance(sink, source)
        if distance > 1.6:
            return False
    ##Visibility Checker
    cmdVisibilityChecker = './algorithms/visibilityChecker/visibilityChecker ' + str(outputDir)
    print cmdVisibilityChecker
    p = subprocess.Popen(cmdVisibilityChecker, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    if p.returncode != 0:
        print >> sys.stderr, 'ERROR: %s' % err
    if out == "TRUE":
        return True
    else:
        heights = [15,20,25,30]
        for h in heights:
            calculateViewsheds(outputDir+"/out.asc", mapMetaData, h, outputDir)
            cmdVisibilityChecker = './algorithms/visibilityChecker/visibilityChecker ' + str(outputDir)
            p = subprocess.Popen(cmdVisibilityChecker, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            out, err = p.communicate()
            print "Check visibility for: ", h, "and the result is", out
            if p.returncode != 0:
                print >> sys.stderr, 'ERROR: %s' % err
            if out == "TRUE":
                return True
    return False



def applyRemoval(outputDir):
    ##SquareRemoval
    cmdSquareBased = './algorithms/square_based/square_removal.py ' + str(outputDir)
    print cmdSquareBased
    p3 = subprocess.Popen(cmdSquareBased, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p3.communicate()
    if p3.returncode != 0:
        print >>sys.stderr, 'ERROR: %s' % err
        sys.exit(-1)

def generateGPSCoordOutput(ouputJsonFile, fileName, mapMetaData):
    finalOutputwithCoordsHandel = open(fileName, "wb")
    with open(ouputJsonFile) as output_file:
        algoOutput = json.load(output_file)
        edgeNumber = 1
        for edge in algoOutput["edges"]:
            finalOutputwithCoordsHandel.write("L 2 1\n")
            finalOutputwithCoordsHandel.write(str(mapMetaData.indexMapping[int(edge["nodes"][0])].lng) + " " + str(mapMetaData.indexMapping[int(edge["nodes"][0])].lat) + "\n")
            finalOutputwithCoordsHandel.write(str(mapMetaData.indexMapping[int(edge["nodes"][1])].lng) + " " + str(mapMetaData.indexMapping[int(edge["nodes"][1])].lat) + "\n")
            finalOutputwithCoordsHandel.write(str(1)+" "+str(edgeNumber)+"\n")
            edgeNumber += 1
    finalOutputwithCoordsHandel.close()


def runFCNF(mapMetaData, outputDir, equipmentsFilePath, isGlobal):
    print "Running FCNF"
    if isGlobal:
        applyRemoval(outputDir)
        removalTypes = ["squareRemoval", "NoRemoval"]
    else:
        removalTypes = ["NoRemoval"]
    unitXDist, unitYDist = mapMetaData.getUnitXAndY()
    for type in removalTypes:
        cmd = './algorithms/FCNF/main_max_cap_withbug '+outputDir+' '+type+' '+str(mapMetaData.rows)+' '+str(mapMetaData.cols)+' '+str(unitXDist)+' '+str(unitYDist) + ' ' + equipmentsFilePath
        print cmd
        p1 = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p1.communicate()
        if p1.returncode != 0:
            print >> sys.stderr, 'ERROR: %s' % err
            sys.exit(-1)
        print out
        with open(outputDir+"/FCNF_result.txt", "a") as FCNFResultFile:
            FCNFResultFile.write(type+"\n")
            FCNFResultFile.write(out+"\n")
        FCNFResultFile.close()
        solutionPath = outputDir + "/FCNF/" + type
        if os.path.isfile(solutionPath + "/solution_1.json"):
            os.remove(solutionPath + "/solution_1.json")
        try:
            resultJSONfiles = [f for f in listdir(solutionPath) if isfile(join(solutionPath, f))]
        except:
            resultJSONfiles = []
        for jsonFCNF in resultJSONfiles:
            resultCoordTextFile = jsonFCNF[:-5]
            generateGPSCoordOutput(solutionPath + "/" + jsonFCNF, solutionPath + "/" + resultCoordTextFile + "_coords.txt", mapMetaData)
            print "Importing resulting graph into GRASS"
            gscript.run_command('v.in.ascii', quiet=QUIET, overwrite=True, input=solutionPath + "/" + resultCoordTextFile + "_coords.txt", output="FCNF" + "_" + type + "_" + resultCoordTextFile, format="standard", flags="n")

tempDirForMapTiles = os.path.expanduser("~/.cache/srtm/")
QUIET = True
scaling = int(sys.argv[3]) #Number of nodes we want to distribute
# scaling = 1600 #Number of nodes we want to distribute

if __name__ == '__main__':
    start_time = time.time()
    inputSinks = "customerNodes/" + sys.argv[1]
    inputSource = "customerNodes/" + sys.argv[2]
    clusterDir = "//"#sys.argv[3]#"/home/cuda/Dropbox/wisp_planning_tool/clustering/kmeans/new_flat_map/10x10x10/"
    inputForPinnning = ""
    inputForPinningTowers = ""
    mainSource = coordinate(0,0,0)
    with open(inputSource, "r") as inputSourceFileHandel:
        for line in inputSourceFileHandel:
            mainSource.lat = float(line.split(",")[0])
            mainSource.lng = float(line.split(",")[1])
    inputSourceFileHandel.close()

    # create output folder
    outputDir = "/home/cuda/output/"+inputSinks.split("/")[1].split(".")[0]+"_rescaled_"+sys.argv[2]
    try:
        os.mkdir(outputDir)
    except:
        print "Output folder exists, overwriting content..."

    q = Queue.Queue()
    #inputSinks, customerLocations = removeDuplicates(inputSinks)
    customerLocations = []
    with open(inputSinks) as inputSinkFile:
        for sink in inputSinkFile:
            sinkRow = sink.rstrip().split(",")
            coord = coordinate(float(sinkRow[0]), float(sinkRow[1]), int(sinkRow[2]))
            coord.revenue = int(sinkRow[3])
            customerLocations.append(coord)
        inputSinkFile.close()

    mapBoundaries = calculateMapBoundries(customerLocations)
    fileNames = downloadMapTiles(customerLocations, mapBoundaries)
    print fileNames
    gscript = myGrass.init(outputDir, tempDirForMapTiles, fileNames)
    globalMapMetaData = metaData(0, 0, 0, 0, 0, 0)

    mapFile = createGlobalMap(gscript, outputDir, fileNames, inputSinks, mapBoundaries, scaling, globalMapMetaData)
    globalMapMetaData = getMetaData(mapFile, globalMapMetaData, outputDir)
    binaryFile = calculateViewsheds(mapFile, globalMapMetaData, 10, outputDir)

    with open(outputDir+"/basicProperties.txt", "w") as clusteringResultHandel:
        ux,uy = globalMapMetaData.getUnitXAndY()
        clusteringResultHandel.write(str(globalMapMetaData.rows)+","+str(globalMapMetaData.cols)+"\n")
        clusteringResultHandel.write(str(ux)+","+str(uy)+"\n")
        clusteringResultHandel.write(outputDir)
    clusteringResultHandel.close()
    createInputFileForAlgorithm(globalMapMetaData, outputDir, customerLocations, mainSource)

    removalFileForAlgo = "noRemoval"
    removalApplied = "0"

    if sys.argv[4] == "c":
        removalApplied = "1"
        #running clustering script
        cmd = './algorithms/clusteringRemoval/clusteringRemoval ' + sys.argv[5] + ' ' + outputDir + ' 0.6'
        print cmd
        p1 = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        while p1.poll() == None:
            nextline = p1.stdout.readline()
            if nextline != '':
                sys.stdout.write(nextline)
                sys.stdout.flush()
        out, err = p1.communicate()
        if p1.returncode != 0:
            print >> sys.stderr, 'ERROR: %s' % err
            sys.exit(-1)
        print out

        gscript.run_command('r.contour', input='patched', overwrite=True, output='buckets_100', step=100 )

        clusteringRemovalFile = open(outputDir + "/clusteringRemovalRemaining.txt", "r")
        totalPoints = len(clusteringRemovalFile.readlines())
        clusteringRemovalFile.close()

        removalFileForAlgo = "clusteringRemovalRemaining"

        #map diameter and other stuff for msp
        if not os.path.exists(outputDir + "/clusteringElevation/" + str(totalPoints)):
            os.makedirs(outputDir + "/clusteringElevation/" + str(totalPoints))
        cmd = './algorithms/mapDiameterCalculation/mapDiameterCalculation ' + str(totalPoints) + " " + str(globalMapMetaData.rows) + ' ' + str(globalMapMetaData.cols) + ' ' + outputDir + "/out_" + str(globalMapMetaData.rows * globalMapMetaData.cols) + ".bin " + outputDir + "/index_mapping.txt " + outputDir+"/clusteringRemovalRemaining.txt clusteringElevation"
        print cmd
        p1 = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p1.communicate()
        if p1.returncode != 0:
            print >> sys.stderr, 'ERROR: %s' % err
            sys.exit(-1)

    if sys.argv[4] == "r":
        removalApplied = "1"
        #randomRemoval
        cmd = './algorithms/randomRemoval/randomRemoval ' + sys.argv[5] + " " + str(globalMapMetaData.rows) + ' ' + str(globalMapMetaData.cols) + ' ' + outputDir+"/out_"+str(globalMapMetaData.rows * globalMapMetaData.cols)+".bin " + outputDir+"/randomRemoval.txt"
        print cmd
        p1 = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p1.communicate()
        if p1.returncode != 0:
            print >> sys.stderr, 'ERROR: %s' % err
            sys.exit(-1)


        #map diameter and stuff for random removal
        randomRemovalFile = open(outputDir+"/randomRemovalRemaining.txt", "r")
        randomRemovalLines = randomRemovalFile.readlines()
        print "RL",len(randomRemovalLines)

        removalFileForAlgo = "randomRemovalRemaining"

        if not os.path.exists(outputDir + "/randomRemoval/" + str(len(randomRemovalLines))):
            os.makedirs(outputDir + "/randomRemoval/" + str(len(randomRemovalLines)))
        cmd = './algorithms/mapDiameterCalculation/mapDiameterCalculation ' + str( len(randomRemovalLines) ) + " " + str(globalMapMetaData.rows) + ' ' + str(globalMapMetaData.cols) + ' ' + outputDir + "/out_" + str(globalMapMetaData.rows * globalMapMetaData.cols) + ".bin " + outputDir + "/index_mapping.txt " + outputDir + "/randomRemovalRemaining.txt randomRemoval"
        print cmd
        p1 = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p1.communicate()
        if p1.returncode != 0:
            print >> sys.stderr, 'ERROR: %s' % err
            sys.exit(-1)

    if sys.argv[4] == "e":
        removalApplied = "1"
        ##ElevationRemoval
        ux, uy = globalMapMetaData.getUnitXAndY()
        cmdFCNF = './algorithms/elevationRemoval/elevationRemoval ' + sys.argv[5] + ' -1 ' + outputDir + "/out.asc " + outputDir + "/out_" + str(int(globalMapMetaData.rows) * int(globalMapMetaData.cols)) + ".bin " + outputDir + "/elevationRemoval.txt 0 " + str(ux) + " " + str(uy)
        print cmdFCNF
        FCNFProcess = subprocess.Popen(cmdFCNF, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        outFCNF, err = FCNFProcess.communicate()
        print outFCNF


        elevationRemovalFile = open(outputDir + "/elevationRemovalRemaining.txt", "r")
        elevationRemovalLines = elevationRemovalFile.readlines()
        print "EL", len(elevationRemovalLines)

        removalFileForAlgo = "elevationRemovalRemaining"

        if not os.path.exists(outputDir + "/elevationRemoval/" + str(len(elevationRemovalLines))):
            os.makedirs(outputDir + "/elevationRemoval/" + str(len(elevationRemovalLines)))
        cmd = './algorithms/mapDiameterCalculation/mapDiameterCalculation ' + str(len(elevationRemovalLines)) + " " + str(globalMapMetaData.rows) + ' ' + str(globalMapMetaData.cols) + ' ' + outputDir + "/out_" + str(globalMapMetaData.rows * globalMapMetaData.cols) + ".bin " + outputDir + "/index_mapping.txt " + outputDir + "/elevationRemovalRemaining.txt elevationRemoval"
        print cmd
        p1 = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p1.communicate()
        if p1.returncode != 0:
            print >> sys.stderr, 'ERROR: %s' % err
            sys.exit(-1)

    if sys.argv[6] == "fcnf":
        cmd = "./algorithms/FCNF/main "+outputDir+" "+removalFileForAlgo+" equipments.csv "+sys.argv[6]+" "+removalApplied
        print cmd
        p1 = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p1.communicate()
        if p1.returncode != 0:
            print >> sys.stderr, 'ERROR: %s' % err
            sys.exit(-1)
        with open(outputDir + "/FCNF_Results.txt", "w") as fcnfResultFile:
            fcnfResultFile.write(out)
        fcnfResultFile.close()


    if sys.argv[6] == "mcf":
        if sys.argv[4] == "n":
            print "MCF Without Removal"
            cmd = "./algorithms/MCF-RR/mcf "+outputDir+" "+removalFileForAlgo+" equipments.csv 0 10 " + str(time.time())
            print cmd
            p1 = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            out, err = p1.communicate()
            if p1.returncode != 0:
                print >> sys.stderr, 'ERROR: %s' % err
                sys.exit(-1)

        else:
            print "MCF With Removal"
            cmd = "./algorithms/MCF-RR/mcf_removal " + outputDir + " " + removalFileForAlgo + " equipments.csv 0 10 " + str(time.time())
            print cmd
            p1 = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            out, err = p1.communicate()
            if p1.returncode != 0:
                print >> sys.stderr, 'ERROR: %s' % err
                sys.exit(-1)
