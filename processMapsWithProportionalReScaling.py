import srtm
import os
from tqdm import tqdm
import json
import sys
import subprocess
from math import cos, sin, asin, sqrt, radians, floor
import myGrass
import time
import math

class coordinate:
    def __init__(self, lat, lng, demand):
        self.lat = float(lat)
        self.lng = float(lng)
        self.demand = int(demand)
        self.revenue = 0
        self.index = 0
        self.mountingHeight = 0


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
    '''
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
    '''
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
            gscript.run_command('r.in.srtm', quiet=QUIET, flags="1", overwrite=True, input=os.path.join(tempDirForMapTiles, file))
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

    print "Exporting the resampled map to", outputDir + '/original_out.asc'
    gscript.run_command('r.out.gdal', quiet=QUIET, overwrite=True, input='patched', output=outputDir + '/original_out.asc', format='AAIGrid')

    with open(outputDir + '/original_out.asc', 'r') as e:
        cols = float(e.readline().strip().split()[1])
        rows = float(e.readline().strip().split()[1])
        e.readline()
        e.readline()
        dx = float(e.readline().strip().split()[1])
        dy = float(e.readline().strip().split()[1])
    e.close()

    scaling_new = scaling#int((cols * rows) * ((100 - scaling) / 100.0))
    aspectRatio = (float)(cols / rows)
    smallCols = aspectRatio
    smallRows = 1
    incrementalDelta = 1
    while (smallCols * smallRows) <= scaling_new:
        smallCols = incrementalDelta * aspectRatio
        smallRows = incrementalDelta * 1
        incrementalDelta += 1
    smallCols = int(smallCols)
    smallRows = int(smallRows)

    print "Resampling"
    gscript.run_command('g.region', flags='a', rows=smallRows, cols=smallCols)

    if customerLocationsFile != "":
        gscript.run_command('v.in.ascii', input=customerLocationsFile, output='sinks', separator='comma', x=2, y=1)

    print "Creating index mapping"
    gscript.run_command('r.to.vect', quiet=QUIET, input='patched', output='index_mapping', type='point')

    print "Exporting index mapping to", outputDir + '/index_mapping.txt'
    gscript.run_command('v.out.ascii', quiet=QUIET, overwrite=True, input='index_mapping',output=outputDir + '/index_mapping.txt', format='point')

    print "Removing elevation < 0m from the map"
    gscript.mapcalc('patched = if(patched > 0, patched, null())', overwrite=True)

    print "Exporting the resampled map to", outputDir + '/out.asc'
    gscript.run_command('r.out.gdal', quiet=QUIET, overwrite=True, input='patched', output=outputDir + '/out.asc', format='AAIGrid')


    return outputDir + '/out.asc'


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

def calculateViewsheds(mapFile, mapMetaData, towerHeight, outputDir, absolutePath):

    print 'Starting CUDA'
    print "thp: double check the metaData"
    startcmd = absolutePath+'/algorithms/CUDA/main '+mapFile + " " + str(mapMetaData.rows * mapMetaData.cols) + " " + str(towerHeight) + " " +outputDir+"/out_"+str(mapMetaData.rows * mapMetaData.cols)+".bin "
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
        nodeProperty = {"type": "sink", "capacity": sink.demand, "mountingHeight": sink.mountingHeight, "frequencies": [2.4, 3, 5, 24],
                        "revenue": sink.revenue}  # randint(1000, 5500)
        coordinates = {"lat": mapMetaData.indexMapping[nearestPointIndex - 1].lat,
                       "lng": mapMetaData.indexMapping[nearestPointIndex - 1].lng}
        totalSource += sink.demand
        jsonDATA.append({"node": int(nearestPointIndex), "nodeProperty": nodeProperty, "coordinates": coordinates})
    nodeProperty = {"type": "source", "capacity": totalSource, "mountingHeight": sourceCoordinates.mountingHeight, "frequencies": [2.4, 3, 5, 24]}
    nearestSourceIndex = getNearestIndex(mapMetaData.indexMapping, sourceCoordinates)
    nearestSourceIndex = assignUniqueSource(mapMetaData, nearestSourceIndex, jsonDATA)
    sourceCoordinates = {"lat": sourceCoordinates.lat, "lng": sourceCoordinates.lng}
    jsonDATA.append({"node": nearestSourceIndex + 1, "nodeProperty": nodeProperty, "coordinates": sourceCoordinates})
    with open(outputDir + '/input.json', "wb") as inputJson:
        inputJson.write(json.dumps({"nodes": jsonDATA}, ensure_ascii=False))
    inputJson.close()

def createPointHeightMapping(customers, outputDir, indexMapping):
    print "creating point - mounting height mapping"
    mappingFileHandel = open(outputDir + "/pointHeightMapping.txt", "w")
    for cust in customers:
        index = getNearestIndex(indexMapping, cust)
        mappingFileHandel.write(str(index) + "," + str(cust.mountingHeight) + "\n")
    mappingFileHandel.close()


def writeInputPointsInRemovalFile(outputDir, removalFileName):
    allNodes = []
    with open(outputDir + "/input.json", "r") as inputJsonFile:
        inputJsonDump = json.load(inputJsonFile)
        for node in inputJsonDump["nodes"]:
            allNodes.append(int(node["node"]) - 1)
    inputJsonFile.close()

    print allNodes

    with open(outputDir + "/" + removalFileName, "a") as removalFile:
        for n in allNodes:
            removalFile.write(str(n) + "\n")
    removalFile.close()


def createPinnedEdgesFile(outputDir,inputEdges, indexMapping):
    pinnedOutputFiles = open(outputDir + "/pinnedEdges.txt", "w")
    with open(inputEdges, "r") as inputEdgesPinnedFile:
        for r in inputEdgesPinnedFile:
            allCoords = r.rstrip().split(",")
            coord1 = coordinate( float(allCoords[0]), float(allCoords[1]), 0 )
            coord2 = coordinate(float(allCoords[2]), float(allCoords[3]), 0)
            tail = getNearestIndex(indexMapping, coord1)
            head = getNearestIndex(indexMapping, coord2)
            pinnedOutputFiles.write(str(tail)+","+str(head)+"\n")
    inputEdgesPinnedFile.close()
    pinnedOutputFiles.close()



tempDirForMapTiles = os.path.expanduser("~/.cache/srtm/")
QUIET = True
scaling = int(sys.argv[3]) #Number of nodes we want to distribute

if __name__ == '__main__':
    absolutePath = sys.argv[7]
    start_time = time.time()
    inputSinks = sys.argv[1]
    inputSource = sys.argv[2]
    inputEdges = sys.argv[9]
    equipmentFilePath = sys.argv[10]
    inputForPinnning = ""
    inputForPinningTowers = ""
    mainSource = coordinate(0,0,0)
    with open(inputSource, "r") as inputSourceFileHandel:
        for line in inputSourceFileHandel:
            mainSource.lat = float(line.split(",")[0])
            mainSource.lng = float(line.split(",")[1])
            mainSource.mountingHeight = int(line.split(",")[2])
    inputSourceFileHandel.close()

    # create output folder
    outputDir = "/home/cuda/output/new/"+inputSinks.split("/")[8].split(".")[0]
    try:
        os.mkdir(outputDir)
    except:
        print "Output folder exists, overwriting content..."

    #inputSinks, customerLocations = removeDuplicates(inputSinks)
    customerLocations = []
    with open(inputSinks) as inputSinkFile:
        for sink in inputSinkFile:
            sinkRow = sink.rstrip().split(",")
            coord = coordinate(float(sinkRow[0]), float(sinkRow[1]), int(sinkRow[2]))
            coord.revenue = int(sinkRow[3])
            coord.mountingHeight = int(sinkRow[4])
            customerLocations.append(coord)
        inputSinkFile.close()

    mapBoundaries = calculateMapBoundries(customerLocations)
    fileNames = downloadMapTiles(customerLocations, mapBoundaries)
    print fileNames
    gscript = myGrass.init(outputDir, tempDirForMapTiles, fileNames)
    globalMapMetaData = metaData(0, 0, 0, 0, 0, 0)

    mapFile = createGlobalMap(gscript, outputDir, fileNames, inputSinks, mapBoundaries, scaling, globalMapMetaData)
    globalMapMetaData = getMetaData(mapFile, globalMapMetaData, outputDir)
    createPointHeightMapping(customerLocations, outputDir, globalMapMetaData.indexMapping)
    binaryFile = calculateViewsheds(mapFile, globalMapMetaData, int(sys.argv[8]), outputDir, absolutePath)

    with open(outputDir+"/basicProperties.txt", "w") as clusteringResultHandel:
        ux, uy = globalMapMetaData.getUnitXAndY()
        clusteringResultHandel.write(str(globalMapMetaData.rows) + "," + str(globalMapMetaData.cols) + "\n")
        clusteringResultHandel.write(str(ux) + "," + str(uy) + "\n")
        clusteringResultHandel.write(outputDir)
    clusteringResultHandel.close()
    customerLocations = customerLocations[:-1]
    createInputFileForAlgorithm(globalMapMetaData, outputDir, customerLocations, mainSource)
    createPinnedEdgesFile(outputDir, inputEdges, globalMapMetaData.indexMapping)

    removalFileForAlgo = "noRemoval"
    removalApplied = "0"

    if sys.argv[4] == "c":
        removalApplied = "1"
        #running clustering script
        cmd = absolutePath+'/algorithms/clusteringRemoval/clusteringRemoval ' + sys.argv[5] + ' ' + outputDir + ' 0.6'
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
        writeInputPointsInRemovalFile(outputDir, "clusteringRemovalRemaining.txt")

        #map diameter and other stuff for msp
        if not os.path.exists(outputDir + "/clusteringElevation/" + str(totalPoints)):
            os.makedirs(outputDir + "/clusteringElevation/" + str(totalPoints))
        cmd = absolutePath+'/algorithms/mapDiameterCalculation/mapDiameterCalculation ' + str(totalPoints) + " " + str(globalMapMetaData.rows) + ' ' + str(globalMapMetaData.cols) + ' ' + outputDir + "/out_" + str(globalMapMetaData.rows * globalMapMetaData.cols) + ".bin " + outputDir + "/index_mapping.txt " + outputDir+"/clusteringRemovalRemaining.txt clusteringElevation"
        print cmd
        p1 = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p1.communicate()
        if p1.returncode != 0:
            print >> sys.stderr, 'ERROR: %s' % err
            sys.exit(-1)

    if sys.argv[4] == "r":
        removalApplied = "1"
        #randomRemoval
        cmd = absolutePath+'/algorithms/randomRemoval/randomRemoval ' + sys.argv[5] + " " + str(globalMapMetaData.rows) + ' ' + str(globalMapMetaData.cols) + ' ' + outputDir+"/out_"+str(globalMapMetaData.rows * globalMapMetaData.cols)+".bin " + outputDir+"/randomRemoval.txt"
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
        writeInputPointsInRemovalFile(outputDir, "randomRemovalRemaining.txt")

        if not os.path.exists(outputDir + "/randomRemoval/" + str(len(randomRemovalLines))):
            os.makedirs(outputDir + "/randomRemoval/" + str(len(randomRemovalLines)))
        cmd = absolutePath+'/algorithms/mapDiameterCalculation/mapDiameterCalculation ' + str( len(randomRemovalLines) ) + " " + str(globalMapMetaData.rows) + ' ' + str(globalMapMetaData.cols) + ' ' + outputDir + "/out_" + str(globalMapMetaData.rows * globalMapMetaData.cols) + ".bin " + outputDir + "/index_mapping.txt " + outputDir + "/randomRemovalRemaining.txt randomRemoval"
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
        cmdFCNF = absolutePath+'/algorithms/elevationRemoval/elevationRemoval ' + sys.argv[5] + ' -1 ' + outputDir + "/out.asc " + outputDir + "/out_" + str(int(globalMapMetaData.rows) * int(globalMapMetaData.cols)) + ".bin " + outputDir + "/elevationRemoval.txt 0 " + str(ux) + " " + str(uy)
        print cmdFCNF
        FCNFProcess = subprocess.Popen(cmdFCNF, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        outFCNF, err = FCNFProcess.communicate()
        print outFCNF


        elevationRemovalFile = open(outputDir + "/elevationRemovalRemaining.txt", "r")
        elevationRemovalLines = elevationRemovalFile.readlines()
        print "EL", len(elevationRemovalLines)

        removalFileForAlgo = "elevationRemovalRemaining"
        writeInputPointsInRemovalFile(outputDir, "elevationRemovalRemaining.txt")

        if not os.path.exists(outputDir + "/elevationRemoval/" + str(len(elevationRemovalLines))):
            os.makedirs(outputDir + "/elevationRemoval/" + str(len(elevationRemovalLines)))
        cmd = absolutePath+'/algorithms/mapDiameterCalculation/mapDiameterCalculation ' + str(len(elevationRemovalLines)) + " " + str(globalMapMetaData.rows) + ' ' + str(globalMapMetaData.cols) + ' ' + outputDir + "/out_" + str(globalMapMetaData.rows * globalMapMetaData.cols) + ".bin " + outputDir + "/index_mapping.txt " + outputDir + "/elevationRemovalRemaining.txt elevationRemoval"
        print cmd
        p1 = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p1.communicate()
        if p1.returncode != 0:
            print >> sys.stderr, 'ERROR: %s' % err
            sys.exit(-1)

    if sys.argv[4] != "n":
        writeInputPointsInRemovalFile(outputDir, removalFileForAlgo + ".txt")

    if sys.argv[6] == "fcnf":
        cmd = absolutePath + "/algorithms/FCNF/main " + outputDir + " " + removalFileForAlgo + " " + equipmentFilePath + " " + removalApplied
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
        print "MCF Without Removal"
        cmd = absolutePath+"/algorithms/MCF-RR/mcf "+outputDir+" "+removalFileForAlgo+" "+equipmentFilePath+" 1 10 98574"
        print cmd
        p1 = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p1.communicate()
        if p1.returncode != 0:
            print >> sys.stderr, 'ERROR: %s' % err
            sys.exit(-1)


    print "All Done"
    with open(outputDir+"/acknowledged.txt", "w") as ackFile:
        ackFile.write("Done\n")
    ackFile.close()

