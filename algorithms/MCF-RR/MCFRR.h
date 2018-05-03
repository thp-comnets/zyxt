//
// Created by cuda on 3/26/18.
//
#include "ortools/base/commandlineflags.h"
#include "ortools/base/logging.h"
#include "ortools/graph/ebert_graph.h"
#include "ortools/graph/max_flow.h"
#include "ortools/graph/min_cost_flow.h"
#include "json.hpp"
#include <sys/time.h>
#include <sys/stat.h>
#include <iostream>
#include <climits>
#include <cfloat>
#include <fstream>
#ifndef MCF_MCF_H
#define MCF_MCF_H


#define EQUIPMENT_LENGTH 9

using namespace std;
using namespace operations_research;
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
    Equipment(){
        this->totalPairCost = 0;
        this->throughput = 0;
        this->range = 0;
        this->id = 0;
    }
};

struct Edge{
    long tail;
    long head;
    int capacity;
    int passedFlow;
    long unitCost;
    Equipment equipment;
    double edgeLength;
    bool isChecked;
    Edge(){
        this->tail = 0;
        this->head = 0;
        this->capacity = 0;
        this->passedFlow = 0;
        this->edgeLength = 0.0;
        this->unitCost = 0;
        this->isChecked = false;
    }
};

struct Solution{
    double solutionNumber;
    double totalPinnedCost;
    double totalOriginalCost;
    double totalEquipmentcost;
    map<string, Edge> solutionEdges;
};

typedef pair<string,Edge> stringEdgePair;

class MCF {
    int runIndex;
    SimpleMinCostFlow simpleMinCostFlow;
    char *mainOutputDir;
    char *removalAlgorithm;
    char *equipmentFilePath;
    int maximumCapacity;
    bool applyRemoval;
    unsigned char* visibilityMatrix;
    long rows;
    long cols;
    GPSPoint* allCoordinates;
    bool* nonRemovedPoints;
    json inputJson;
    double edgeDistanceScaling;
    Equipment *equipmentList;
    map<int, int> sourceMap;
    map<int, int> sinksMap;
    map<string, Edge> edgesKept;
    vector<Solution> allSolutions;
    long totalNetworkEdges;
    double calculateDistance(GPSPoint, GPSPoint);
    Equipment getBestDevice(GPSPoint, GPSPoint, int);
    short getBit(unsigned long long int);
    double deg2rad(double);
    void initializePreliminaryData();
    void writeJSONOutput(const SimpleMinCostFlow &minCostFlow, int);
    double getEdgeCost(double);
    double calculateEquipmentCostOnly(const SimpleMinCostFlow &minCostFlow);
    bool isEdgeWithLeafNode(string edge);

public:
    void clearMetaData();
    void setRunIndex(int runIndex);

    void setEdgesKept(const map<string, Edge> &edgesKept);

    void pushSolution(Solution );

    const map<string, Edge> &getEdgesKept() const;
    void furtherOptimizeMinSolution(int);
    vector<Solution>& getAllSolutions();
    MCF(char*, char*, char*, bool, double);
    void populateModel(double); // first argument is visibility range, 0 for all
    void solve();
    int solveInIterations(int);
    void optimizeMinSolution(int);
    ~MCF();
};


#endif //MCF_MCF_H
