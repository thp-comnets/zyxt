#include <iostream>
#include <fstream>
#include <cfloat>
#include <cmath>
#include <sstream>
#include <vector>
#include "Graph.cpp"
using namespace std;



void readCoordinateMapping(char *indexMappingFilePath, Coordinate* allCoordinates, long size, int lineSize){
    string line;
    ifstream indexMappingFileHandel(indexMappingFilePath);
    long ind = 0;
    while (indexMappingFileHandel >> line && ind < size){
        stringstream ss(line);
        string token;
        if(lineSize > 1) {
            getline(ss, token, '|');
            allCoordinates[ind].lng = stod(token);
            getline(ss, token, '|');
            allCoordinates[ind].lat = stod(token);
        }
        getline(ss, token, '|');
        allCoordinates[ind].index = stoi(token);
        ind++;
    }
    indexMappingFileHandel.close();
}


double deg2rad(double deg) {
    return deg * (M_PI/180);
}


double calculateDistance(Coordinate p1, Coordinate p2){
    double R = 6371; // Radius of the earth in km
    double dLat = deg2rad(p2.lat-p1.lat);  // deg2rad below
    double dLon = deg2rad(p2.lng-p1.lng);
    double a = sin(dLat/2) * sin(dLat/2) + cos(deg2rad(p1.lat)) * cos(deg2rad(p2.lat)) * sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return (R * c); // Distance in km
}


short getBit(unsigned char *A, unsigned long long int k) {
    return ( (A[k/8] & (1 << (k%8) )) != 0 ) ;
}
void readBinaryFile(unsigned char* &visibilityMatrix, long rows, char* binaryFile){
    cout << "Start Reading Visibility Matrix" << endl;
    visibilityMatrix = (unsigned char *)calloc(ceil((float)(rows * rows) / 8) * sizeof(unsigned char), sizeof(unsigned char));
    FILE* fstr = fopen(binaryFile, "rb");
    fread(visibilityMatrix, sizeof(unsigned char), ceil((float)(rows * rows) / 8) * sizeof(unsigned char), fstr);
    fclose(fstr);
}


int main(int argc, char *argv[]) {

    long remainingPoints = stol(argv[1]);
    long visibilityMatrixNodes = stol(argv[2]) * stol(argv[3]);
    Graph g(remainingPoints);
    unsigned char *visibilityMatrix = NULL;
    cout << "Total Nodes: " << visibilityMatrixNodes << endl;
    cout << "Remaining Nodes: " << remainingPoints << endl;
    readBinaryFile(visibilityMatrix, visibilityMatrixNodes, argv[4]);
    ///
    Coordinate *indexMapping = (Coordinate*)malloc(visibilityMatrixNodes * sizeof(Coordinate));
    Coordinate *remainingPointsList = (Coordinate*)malloc(remainingPoints * sizeof(Coordinate));
    readCoordinateMapping(argv[5], indexMapping, visibilityMatrixNodes, 3);
    readCoordinateMapping(argv[6], remainingPointsList, remainingPoints, 1);

    char appendedNewName[20] = "WithCoords.txt\0";
    char nonRemovedFilePathWithCoords[300] = {'\0'};
    strcpy(nonRemovedFilePathWithCoords, argv[6]);
    nonRemovedFilePathWithCoords[strlen(nonRemovedFilePathWithCoords)-4] = '\0';
    strcat(nonRemovedFilePathWithCoords, appendedNewName);

    ofstream remainingPointWithCoordsHandel(nonRemovedFilePathWithCoords);
    remainingPointWithCoordsHandel.precision(15);
    for(int i = 0; i <  remainingPoints; i++){
        remainingPointWithCoordsHandel << indexMapping[ remainingPointsList[i].index ].lng << "," << indexMapping[ remainingPointsList[i].index ].lat <<
        "," << indexMapping[ remainingPointsList[i].index ].index << endl;
    }
    remainingPointWithCoordsHandel.close();
    int totalEdges = 0;
    for(long i = 0; i < remainingPoints; i++){
        for(long j = 0; j < remainingPoints; j++){
            if(getBit(visibilityMatrix, remainingPointsList[i].index * visibilityMatrixNodes + remainingPointsList[j].index)){
                double distance = calculateDistance(indexMapping[ remainingPointsList[i].index ], indexMapping[ remainingPointsList[j].index ]);

                //if(distance <= 0.3) {
                    totalEdges++;
                    g.addEdge(i, j, distance);
                //}
            }
        }
    }
    cout << "Graph Loaded... With Edges: " << totalEdges << endl;


    char appendedNewName2[15] = "Result.txt\0";
    char resultFilePath[300] = {'\0'};
    strcpy(resultFilePath, argv[6]);
    resultFilePath[strlen(resultFilePath)-4] = '\0';
    strcat(resultFilePath, appendedNewName2);
    cout << "Results Will Be At: " << resultFilePath << endl;
    ofstream resultFileHandel(resultFilePath);


    bool *globalVisitedNodes = new bool[remainingPoints]();

    for(int i = 0; i < remainingPoints; i++){
        if(!globalVisitedNodes[i]){

            ////
            char appendedNewName3[100] = {'\0'};
            sprintf(appendedNewName3, "%s/%li/TreeFrom_%d.txt", argv[7],remainingPoints, i);
            char treeOutputFilePath[300] = {'\0'};
            strcpy(treeOutputFilePath, argv[5]);
            for(int i = strlen(treeOutputFilePath)-1; i >= 0; i--){
                if(treeOutputFilePath[i] == '/'){
                    break;
                }
                treeOutputFilePath[i] = '\0';
            }

            strcat(treeOutputFilePath, appendedNewName3);
            cout << "Tree Will Be At: " << treeOutputFilePath << endl;
            ////

            long vCount = g.BFSV2(i, globalVisitedNodes, indexMapping, remainingPointsList, treeOutputFilePath);
            cout << "Visited From: " << i << " is " << vCount-1 << endl;
            resultFileHandel << i << "," << vCount-1 << " / " << remainingPoints << endl;
        }
    }
    // printf("Start Calculating The Longest Shortest Path\n");
    // shortestPathResult longestShortestPath;
    // longestShortestPath.hops = INT_MIN;
    // for(int i = 0; i < remainingPoints; i++){
    //     shortestPathResult shortestPathResult1 = g.shortestPath(i);
    //     if(shortestPathResult1.hops > longestShortestPath.hops){
    //         longestShortestPath.hops = shortestPathResult1.hops;
    //         longestShortestPath.source = shortestPathResult1.source;
    //         longestShortestPath.to = shortestPathResult1.to;
    //         longestShortestPath.dist = shortestPathResult1.dist;
    //     }
    // }
    // resultFileHandel << endl;
    // cout << longestShortestPath.source << " => " << longestShortestPath.to << ", Dist: " << longestShortestPath.dist << ", Hops: " << longestShortestPath.hops << endl;
    // resultFileHandel << longestShortestPath.source << " => " << longestShortestPath.to << ", Dist: " << longestShortestPath.dist << ", Hops: " << longestShortestPath.hops << endl;
    resultFileHandel << endl;
    resultFileHandel << 0 << " => " << 0 << ", Dist: " << 0 << ", Hops: " << 0 << endl;

    int covered_nodes = 0;
    for(long j = 0; j < visibilityMatrixNodes; j++){
        for(long i = 0; i < remainingPoints; i++){
            double distance = calculateDistance(indexMapping[ remainingPointsList[i].index ], indexMapping[j]);
            if(/*distance <= 0.3 && */getBit(visibilityMatrix, remainingPointsList[i].index * visibilityMatrixNodes + j)){
                covered_nodes++;
                break;
            }
        }
    }
    cout << "Covered Nodes: " << covered_nodes << endl;
    cout.precision(5);
    double percentCovered = ((double)covered_nodes / (double(visibilityMatrixNodes))) * 100.0;
    cout << percentCovered << " Coverage" << endl;
    resultFileHandel << endl;
    resultFileHandel << percentCovered << " Coverage" << endl;



    delete []globalVisitedNodes;
    resultFileHandel.close();
    free(visibilityMatrix);
    free(indexMapping);
    free(remainingPointsList);



    return 0;
}