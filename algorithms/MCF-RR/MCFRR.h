//
// Created by cuda on 4/24/18.
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
#ifndef NEWMCF_MCFRR_H
#define NEWMCF_MCFRR_H

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
    double totalEquipmentCost;
    Solution(){
        this->totalEquipmentCost = 0;
        this->totalPinnedCost = 0;
        this->totalOriginalCost = 0;
        this->totalEquipmentCost = 0;
    }
    map<string, Edge> solutionEdges;
};

typedef pair<string,Edge> stringEdgePair;


class MCFRR {
    int runIndex;
    char *mainOutputDir;
    char *removalAlgorithm;
    char *equipmentFilePath;
    unsigned char* visibilityMatrix;
    long rows;
    long cols;
    GPSPoint* allCoordinates;
    vector<long> networkNodes;
    json inputJson;
    int edgeDistanceScaling;
    Equipment *equipmentList;
    map<long, int> sourceMap;
    map<long, int> sinksMap;
    map<long, int> heightMap;
    map<string, Edge> edgesKept;
    map<string, Edge> pinnedEdges;
    Solution currentSolution;
    long totalNetworkEdges;
    double calculateDistance(GPSPoint, GPSPoint);
    Equipment getBestDevice(GPSPoint, GPSPoint, int);
    short getBit(unsigned long long int);
    double deg2rad(double);
    void initializePreliminaryData();
    void writeJSONOutput(const SimpleMinCostFlow &minCostFlow, int);
    double getEdgeUnitCost(double);
    double calculateEquipmentCostOfNetwork(const SimpleMinCostFlow &minCostFlow);
    bool isNetworkHasPinnedEdges(const SimpleMinCostFlow &minCostFlow);
    long getNodeLocalIndex(long node);
    long getIndexFromNetworkNodesVector(long node);
    bool isEdgeWithLeafNode(string edge);
public:
    MCFRR(char *mainOutputDir, char *removalAlgorithm, char *equipmentFilePath, int edgeDistanceScaling);
    void clearMetaData();
    const map<string, Edge> &getEdgesKept() const;
    void doFurtherOptimization(bool);
    int solveInIterations(unsigned long);
    void setRunIndex(int runIndex);
    void setEdgesKept(const map<string, Edge> &edgesKept);
    void setPreviousSolution(const Solution &previousSolution);
    const Solution &getPreviousSolution() const;
    virtual ~MCFRR();
};


#endif //NEWMCF_MCFRR_H
