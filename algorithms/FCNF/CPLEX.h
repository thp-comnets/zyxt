//
// Created by cuda on 3/6/18.
//
#include <ilcplex/ilocplex.h>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sstream>
#include "json.hpp"

#ifndef FCNF_CPLEX_H
#define FCNF_CPLEX_H

#define EQUIPMENT_LENGTH 9

using namespace std;
using json = nlohmann::json;

struct GPSPoint{
    double lat;
    double lng;
};

struct Equipment{
    int throughput;
    int range; // in KM
    int totalPairCost;
    int id;
};

struct NodeEdgeRef{
    IloNumVarArray inComing;
    IloNumVarArray outGoing;
};

class CPLEX {
    char *mainOutputDir;
    char *removalAlgorithm;
    char *equipmentFilePath;
    int maximumCapacity;
    bool applyRemoval;
    unsigned char* visibilityMatrix;
    long rows;
    long cols;
    double cplexTimeLimit;
    IloEnv env;
    GPSPoint* allCoordinates;
    bool* nonRemovedPoints;
    IloModel model;
    IloNumVarArray numVarArray;
    IloBoolVarArray boolVarArray;
    IloRangeArray con;
    IloCplex cplex;
    json inputJson;
    double edgeDistanceScaling;
    Equipment *equipmentList;
    map<int, int> sourceMap;
    map<int, int> sinksMap;
    long totalNetworkEdges;
    vector<int> splitStringAndGetEdges(const string&);
    double calculateDistance(GPSPoint, GPSPoint);
    Equipment getBestDevice(GPSPoint, GPSPoint, int);
    short getBit(unsigned long long int);
    double deg2rad(double);
    void initializePreliminaryData();
public:
    CPLEX(char*, char*, char*, double, bool, double);
    void populateModel(double); // first argument is visibility range, 0 for all
    void writeModel(const char *);
    void solve();
    ~CPLEX();
};



#endif //FCNF_CPLEX_H
