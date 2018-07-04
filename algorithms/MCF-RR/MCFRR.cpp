//
// Created by cuda on 4/24/18.
//

#include "MCFRR.h"
MCFRR::MCFRR(char *mainOutputDir, char *removalAlgorithm, char *equipmentFilePath, int edgeDistanceScaling) {
    this->mainOutputDir = mainOutputDir;
    this->removalAlgorithm = removalAlgorithm;
    this->equipmentFilePath = equipmentFilePath;
    this->equipmentList = new Equipment[EQUIPMENT_LENGTH];
    this->totalNetworkEdges = 0;
    this->edgeDistanceScaling = edgeDistanceScaling;
    initializePreliminaryData();
}

short MCFRR::getBit(unsigned long long int k) {
    return ((this->visibilityMatrix[k / 8] & (1 << (k % 8))) != 0);
}

double MCFRR::deg2rad(double deg) {
    return deg * (M_PI/180);
}

double MCFRR::calculateDistance(GPSPoint p1, GPSPoint p2) {
    double R = 6371; // Radius of the earth in km
    double dLat = deg2rad(p2.lat-p1.lat);  // deg2rad below
    double dLon = deg2rad(p2.lng-p1.lng);
    double a = sin(dLat/2) * sin(dLat/2) + cos(deg2rad(p1.lat)) * cos(deg2rad(p2.lat)) * sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return (R * c) * this->edgeDistanceScaling; // Distance in km
}

double MCFRR::getEdgeUnitCost(double edgeLength) {
    return ( ( 0.1258 * pow(edgeLength, 2) ) + (6.7747 * edgeLength) + 63.429 );
}

Equipment MCFRR::getBestDevice(GPSPoint p1, GPSPoint p2, int capacityToCompare) {
    double distance = calculateDistance(p1,p2);
    int cost = INT_MAX;
    Equipment selEq;
    selEq.id = -1;
    for (int k = 0; k < EQUIPMENT_LENGTH; k++){
        if(this->equipmentList[k].range >= distance && this->equipmentList[k].totalPairCost < cost && this->equipmentList[k].throughput >= capacityToCompare){
            cost = this->equipmentList[k].totalPairCost;
            selEq = this->equipmentList[k];
        }
    }
    return selEq;
}

void MCFRR::clearMetaData() {
    this->edgesKept.clear();
    for (auto const& x : this->pinnedEdges){
        this->edgesKept[x.first] = x.second;
    }
    this->currentSolution.totalOriginalCost = 0.0;
    this->currentSolution.totalPinnedCost = 0.0;
    this->currentSolution.solutionNumber = 0.0;
    this->currentSolution.totalEquipmentCost = 0.0;
    this->currentSolution.solutionEdges.clear();
}

double MCFRR::calculateEquipmentCostOfNetwork(const SimpleMinCostFlow &minCostFlow) {
    double totalEquipmentCost = 0.0;
    for(int i = 0; i < minCostFlow.NumArcs(); i++) {
        if (minCostFlow.Flow(i) > 1) {
            Equipment equipment = this->getBestDevice(this->allCoordinates[ this->networkNodes[minCostFlow.Tail(i)] ], this->allCoordinates[ this->networkNodes[minCostFlow.Head(i)] ], minCostFlow.Flow(i));
            totalEquipmentCost += equipment.totalPairCost;
        }
    }
    return totalEquipmentCost;
}

bool MCFRR::isNetworkHasPinnedEdges(const SimpleMinCostFlow &minCostFlow){
    for (auto const& x : this->pinnedEdges){
        bool isEdgeFound = false;
        for(int i = 0; i < minCostFlow.NumArcs(); i++) {
            if(minCostFlow.Flow(i) > 1){
                string edge = to_string(minCostFlow.Tail(i)) + "," + to_string(minCostFlow.Head(i));
                if(x.first == edge){
                    isEdgeFound = true;
                }
            }
        }
        if(!isEdgeFound){
            return false;
        }
    }
    return true;
}


void MCFRR::setRunIndex(int runIndex) {
    MCFRR::runIndex = runIndex;
}
const map<string, Edge> &MCFRR::getEdgesKept() const {
    return this->edgesKept;
}

void MCFRR::setEdgesKept(const map<string, Edge> &edgesKept) {
    MCFRR::edgesKept = edgesKept;
}


long MCFRR::getNodeLocalIndex(long node) {
    for(long i = 0; i < this->networkNodes.size(); i++){
        if(this->networkNodes[i] == node){
            return i;
        }
    }
    return -1;
}

long MCFRR::getIndexFromNetworkNodesVector(long node){
    vector<long>::iterator itr = std::find(this->networkNodes.begin(), this->networkNodes.end(), node);
    if (itr != this->networkNodes.cend()) {
        return std::distance(this->networkNodes.begin(), itr);
    }
    return -1;
}


void MCFRR::initializePreliminaryData() {
    string line;
    char filePathBuffer[200];
    //reading map properties
    filePathBuffer[0] = '\0';
    sprintf(filePathBuffer, "%s/basicProperties.txt", this->mainOutputDir);
    ifstream mapPropFile(filePathBuffer);
    mapPropFile >> line; ///read first line
    stringstream ss(line);
    string token;
    getline(ss, token, ',');
    this->rows = stol(token);
    getline(ss, token, ',');
    this->cols = stol(token);
    mapPropFile.close();
    cout << "Total Map Size: " << this->rows << " X " << this->cols << endl;
    //initializing memories
    this->allCoordinates = new GPSPoint[this->rows * this->cols];
    unsigned long long int totalNodesForVisibilityMatrix = (unsigned long long int)this->rows * this->cols;

    //reading Index Mapping

    sprintf(filePathBuffer, "%s/index_mapping.txt", this->mainOutputDir);
    ifstream indexMappingFileHandel(filePathBuffer);
    int ind = 0;
    while (indexMappingFileHandel >> line){
        stringstream ss(line);
        string token;
        getline(ss, token, '|');
        allCoordinates[ind].lng = stod(token);
        getline(ss, token, '|');
        allCoordinates[ind].lat = stod(token);
        getline(ss, token, '|');
        ind++;
    }
    indexMappingFileHandel.close();

    //reading removal file
    long nodeIndex = -1;
    filePathBuffer[0] = '\0';
    sprintf(filePathBuffer, "%s/%s.txt", this->mainOutputDir, this->removalAlgorithm);
    ifstream fin(filePathBuffer);
    if(!fin.good()){
        for(long i = 0; i < totalNodesForVisibilityMatrix; i++){
            this->networkNodes.push_back(i);
        }
    }else{
        while(fin >> nodeIndex){
            this->networkNodes.push_back(nodeIndex);
        }
    }
    fin.close();

    //reading input json
    filePathBuffer[0] = '\0';
    sprintf(filePathBuffer, "%s/input.json", this->mainOutputDir);
    ifstream inputJsonHandel(filePathBuffer);
    inputJsonHandel >> this->inputJson;
    if (!this->inputJson.empty()){
        for (json::iterator it = inputJson["nodes"].begin(); it != inputJson["nodes"].end(); ++it) {
            if (it[0]["nodeProperty"]["type"] == "source")
            {
                int source = int(it[0]["node"]) - 1;
                long localSourceIndex = this->getNodeLocalIndex(source);
                this->sourceMap[localSourceIndex] = int(it[0]["nodeProperty"]["capacity"]);
                this->heightMap[localSourceIndex] = int(it[0]["nodeProperty"]["mountingHeight"]);
            } else {
                int sinkNode = int(it[0]["node"]) - 1;
                long localSinkIndex = this->getNodeLocalIndex(sinkNode);
                int capacity = int(it[0]["nodeProperty"]["capacity"]) * -1;
                this->sinksMap[localSinkIndex] += capacity;
                this->heightMap[localSinkIndex] = int(it[0]["nodeProperty"]["mountingHeight"]);
            }
        }
    }
    inputJsonHandel.close();

    //readingPinnedEdgesInput
    filePathBuffer[0] = '\0';
    sprintf(filePathBuffer, "%s/pinnedEdges.txt", this->mainOutputDir);
    ifstream inputPinnedEdgesHandel(filePathBuffer);
    line = "";
    while(inputPinnedEdgesHandel >> line){
        Edge newEdge;
        stringstream ss(line);
        string token;
        getline(ss, token, ',');
        newEdge.tail = this->getIndexFromNetworkNodesVector(stol(token)); //doing this to get the local index relative to the list of selected nodes after node removal algorithm
        getline(ss, token, ',');
        newEdge.head = this->getIndexFromNetworkNodesVector(stol(token)); //doing this to get the local index relative to the list of selected nodes after node removal algorithm
        this->pinnedEdges[line] = newEdge;
        this->edgesKept[line] = newEdge;
    }
//     for (auto const& x : this->pinnedEdges)
// {
//     std::cout << x.first  // string (key)
//               << ':'
//               << x.second.tail << "," << x.second.head// string's value
//               << std::endl ;
// }
//     exit(1);
    //reading equipments
    ifstream equipmentFile ( this->equipmentFilePath );
    int i = 0;
    while (equipmentFile.good() && i < EQUIPMENT_LENGTH){
        getline ( equipmentFile, line );
        stringstream ss(line);
        string token;
        getline ( ss, token , ',' );
        // skip first column
        getline(ss, token, ',');
        equipmentList[i].throughput = stoi(token);
        getline(ss, token, ',');
        equipmentList[i].range = stoi(token);
        getline(ss, token, ',');
        equipmentList[i].totalPairCost = stoi(token);
        getline(ss, token, ',');
        equipmentList[i].id = stoi(token);
        i++;
    }
    equipmentFile.close();

    //reading Binary File
    this->visibilityMatrix = (unsigned char *)calloc(ceil((float)(totalNodesForVisibilityMatrix * totalNodesForVisibilityMatrix) / 8) * sizeof(unsigned char), sizeof(unsigned char));
    filePathBuffer[0] = '\0';
    sprintf(filePathBuffer, "%s/out_%llu.bin", this->mainOutputDir, totalNodesForVisibilityMatrix);
    FILE* fstr = fopen(filePathBuffer, "rb");
    fread(this->visibilityMatrix, sizeof(unsigned char), ceil((float)(totalNodesForVisibilityMatrix * totalNodesForVisibilityMatrix) / 8) * sizeof(unsigned char), fstr);
    fclose(fstr);
}


void MCFRR::writeJSONOutput(const SimpleMinCostFlow &minCostFlow, int iter) {
    char nameBuffer[300] = {'\0'};
    json output;
    output["bandwidth"] = 12323232;
    output["time"] = time(0);
    double originalCost = 0.0;
    double totalCost = 0.0;
    double totalEquipmentCost = 0.0;
    int outputCounter = 0;
    map<int, int> nodes;
    this->currentSolution.solutionEdges.clear();
    for(int i = 0; i < minCostFlow.NumArcs(); i++){
        if(minCostFlow.Flow(i) > 0){
            Edge edge;
            Equipment equipment = this->getBestDevice(this->allCoordinates[this->networkNodes[minCostFlow.Tail(i)]], this->allCoordinates[ this->networkNodes[minCostFlow.Head(i)] ], minCostFlow.Flow(i));
            output["edges"][outputCounter]["nodes"] = json::array({this->networkNodes[minCostFlow.Tail(i)], this->networkNodes[minCostFlow.Head(i)]});
            //output["edges"][outputCounter]["coordinates"] = json::array({this->allCoordinates[ this->networkNodes[minCostFlow.Tail(i)] ].lat,this->allCoordinates[ this->networkNodes[minCostFlow.Tail(i)] ].lng, this->allCoordinates[ this->networkNodes[minCostFlow.Head(i)] ].lat, this->allCoordinates[ this->networkNodes[minCostFlow.Head(i)] ].lng });
            output["edges"][outputCounter]["edgeProperty"]["unitCost"] = minCostFlow.UnitCost(i);
            output["edges"][outputCounter]["edgeProperty"]["deviceId"] = equipment.id;
            output["edges"][outputCounter]["edgeProperty"]["bandwidth"] = equipment.throughput;//minCostFlow.Capacity(i);
            output["edges"][outputCounter]["edgeProperty"]["frequency"] = "2.4 GHz";
            output["edges"][outputCounter]["edgeProperty"]["flowPassed"] = minCostFlow.Flow(i);
            double distance = calculateDistance(this->allCoordinates[ this->networkNodes[minCostFlow.Tail(i)] ], this->allCoordinates[ this->networkNodes[minCostFlow.Head(i)] ]);
            double originalUnitCost = this->getEdgeUnitCost(distance);
            output["edges"][outputCounter]["edgeProperty"]["deviceCost"] = equipment.totalPairCost;
            nameBuffer[0] = '\0';
            sprintf(nameBuffer, "%.2f Km", distance);
            output["edges"][outputCounter]["edgeProperty"]["length"] = nameBuffer;
            nodes[this->networkNodes[minCostFlow.Head(i)] ] = minCostFlow.Supply(minCostFlow.Head(i));
            nodes[this->networkNodes[minCostFlow.Tail(i)] ] = minCostFlow.Supply(minCostFlow.Tail(i));
            edge.tail = minCostFlow.Tail(i);
            edge.head = minCostFlow.Head(i);
            edge.capacity = equipment.throughput;//minCostFlow.Capacity(i);
            edge.passedFlow = minCostFlow.Flow(i);
            edge.edgeLength = distance;
            edge.unitCost = minCostFlow.UnitCost(i);
            edge.equipment = equipment;
            this->currentSolution.solutionEdges[to_string(edge.tail)+","+to_string(edge.head)] = edge;
            if(this->edgesKept.count(to_string(edge.tail)+","+to_string(edge.head)) > 0){
                this->edgesKept[to_string(edge.tail)+","+to_string(edge.head)].passedFlow = minCostFlow.Flow(i);
            }
            originalCost += originalUnitCost;
            totalCost +=  minCostFlow.UnitCost(i);
            totalEquipmentCost += equipment.totalPairCost;
            outputCounter++;
        }
    }
    this->currentSolution.solutionNumber = iter;
    this->currentSolution.totalEquipmentCost = round(totalEquipmentCost);
    this->currentSolution.totalOriginalCost = round(originalCost);
    this->currentSolution.totalPinnedCost = totalCost;
    outputCounter = 0;
    for (auto &node : nodes) {
        output["nodes"][outputCounter]["node"] = node.first;
        output["nodes"][outputCounter]["nodeProperty"]["capacity"] = node.second;
        output["nodes"][outputCounter]["nodeProperty"]["mountingHeight"] = this->heightMap[node.first];
        output["nodes"][outputCounter]["nodeProperty"]["lat"] = this->allCoordinates[node.first].lat;
        output["nodes"][outputCounter]["nodeProperty"]["lng"] = this->allCoordinates[node.first].lng;
        if (node.second < 0)
        {
            output["nodes"][outputCounter]["nodeProperty"]["type"] = "sink";
        } else if (node.second > 0)
        {
            output["nodes"][outputCounter]["nodeProperty"]["type"] = "source";
        } else {
            output["nodes"][outputCounter]["nodeProperty"]["type"] = "intermediate";
        }
        outputCounter++;
    }
    output["costs"] = round(totalCost);
    output["originalCost"] = round(originalCost);
    output["equipmentCost"] = round(totalEquipmentCost);
    cout << "Cost For Iteration " << iter << " is: " << round(totalCost) << endl;
    cout << "Original Cost For Iteration " << iter << " is: " << round(originalCost) << endl;
    cout << "Equipment Cost For Iteration " << iter << " is: " << round(totalEquipmentCost) << endl;
    nameBuffer[0] = '\0';
    sprintf(nameBuffer, "%s/MCF/", this->mainOutputDir);
    mkdir(nameBuffer, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    nameBuffer[0] = '\0';
    sprintf(nameBuffer, "%s/MCF/%s_%d", this->mainOutputDir, this->removalAlgorithm, this->runIndex);
    mkdir(nameBuffer, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    nameBuffer[0] = '\0';
    if(iter == -1){
        sprintf(nameBuffer, "%s/MCF/%s_%d/final_1_2.json",this->mainOutputDir, this->removalAlgorithm, this->runIndex);
    }else if(iter == -2){
        sprintf(nameBuffer, "%s/MCF/%s_%d/final_2_2.json",this->mainOutputDir, this->removalAlgorithm, this->runIndex);
    }else{
        sprintf(nameBuffer, "%s/MCF/%s_%d/output_%d.json",this->mainOutputDir, this->removalAlgorithm, this->runIndex, iter);
    }
    ofstream outputJsonFile(nameBuffer);
    outputJsonFile << setw(4) << output;
    outputJsonFile.close();
}


bool MCFRR::isEdgeWithLeafNode(string edge) {
    stringstream ss(edge);
    vector<long> nodesInEdge;
    while(ss.good()){
        string subString;
        getline( ss, subString, ',' );
        nodesInEdge.push_back( stol(subString) );
    }
    // cout << "Node to check: " << nodesInEdge[0] << "::" << nodesInEdge[1] << endl;
    // cout << "*****************"<<endl;
    for(auto eIt = this->currentSolution.solutionEdges.begin(); eIt !=  this->currentSolution.solutionEdges.end(); ++eIt){
        // cout << "IC: " << eIt->second.head << "::" << eIt->second.tail << endl;
        if(nodesInEdge[1] == eIt->second.tail){
            // cout << "IC: TRUE" << endl;
            return false;
        }
    }
    cout << "*****************"<<endl;
    return true;
}


void MCFRR::doFurtherOptimization(bool onlyEdgesWithLeafNodes) {
    cout << "Start Further Optimizing Min Solution" << endl;
    bool isFurtherOptimization;
    char filePathBuffer[200] = {'\0'};
    sprintf(filePathBuffer, "%s/costCompFile.txt", this->mainOutputDir);
    ofstream costCompFile(filePathBuffer);
    do{
        isFurtherOptimization = false;
        vector<stringEdgePair> sortedKeptEdgeList;
        copy(this->edgesKept.begin(),this->edgesKept.end(), std::back_inserter<std::vector<stringEdgePair>>(sortedKeptEdgeList));
        sort(sortedKeptEdgeList.begin(), sortedKeptEdgeList.end(),
             [](const stringEdgePair& l, const stringEdgePair& r) {
                 if (l.second.passedFlow != r.second.passedFlow){
                     return l.second.passedFlow < r.second.passedFlow;
                 }

                 return l.first < r.first;
             });
        // for (auto it = sortedKeptEdgeList.begin(); it != sortedKeptEdgeList.end(); ++it) {
        //     std::cout <<"{"<< it->first << "::" << it->second.passedFlow<<"}" << endl;
        // }
        for(auto it = sortedKeptEdgeList.begin(); it != sortedKeptEdgeList.end(); ++it) {
            if(this->currentSolution.solutionEdges.count(it->first) == 0){
                continue;
            }
            if(onlyEdgesWithLeafNodes && !this->isEdgeWithLeafNode(it->first)){
                cout << "The edge: " << it->first << "::" << it->second.passedFlow << " is skipped in Part-1 of Phase 2. " << endl;
                continue;
            }
            if(!onlyEdgesWithLeafNodes && this->isEdgeWithLeafNode(it->first)){
                cout << "The edge: " << it->first << "::" << it->second.passedFlow << " is skipped in Part-2 of Phase 2. " << endl;
                continue;
            }
            this->edgesKept[it->first].isChecked = true;
            if(this->pinnedEdges.count(it->first) > 0){
                cout << "The edge: " << it->first << "::" << it->second.passedFlow << " is already pinned" << endl;
                continue;
            }
            this->currentSolution.solutionEdges[it->first].isChecked = true;
            SimpleMinCostFlow minCostFlow;
            unsigned long long int totalNodes = (unsigned long long int) this->rows * this->cols;
            for (long i = 0; i < this->networkNodes.size(); i++) {
                for (long j = 0; j < this->networkNodes.size(); j++) {
                    if(this->pinnedEdges.count(to_string(i)+","+to_string(j)) > 0){
                        minCostFlow.AddArcWithCapacityAndUnitCost(i,j,INT_MAX, 0);
                    }else if (getBit(this->networkNodes[i] * totalNodes + this->networkNodes[j])) {
                        if(i == j){
                            continue;
                        }
                        double distance = this->calculateDistance(this->allCoordinates[ this->networkNodes[i] ], this->allCoordinates[ this->networkNodes[j] ]);
                        double unitCost = this->getEdgeUnitCost(distance);
                        if ( (this->edgesKept.count(to_string(i) + "," + to_string(j)) > 0 && !this->edgesKept[to_string(i) + "," + to_string(j)].isChecked) ) {
                            unitCost = 0;
                        } else if (this->currentSolution.solutionEdges.count(to_string(i) + "," + to_string(j)) > 0 && !this->currentSolution.solutionEdges[to_string(i) + "," + to_string(j)].isChecked) {
                            Edge existingEdge = this->currentSolution.solutionEdges[to_string(i) + "," + to_string(j)]; //solutionEdgeMap Hold the solution of previous iteration.
                            if (existingEdge.passedFlow > 5) {
                                this->edgesKept[to_string(i) + "," + to_string(j)] = existingEdge; //Edges Kept
                                unitCost = 0;
                            }
                        }
                        minCostFlow.AddArcWithCapacityAndUnitCost(i, j, INT_MAX, unitCost);
                    }
                }
            }
            for (auto &it : this->sourceMap) {
                minCostFlow.SetNodeSupply(it.first, it.second);
            }
            for (auto &it : this->sinksMap) {
                minCostFlow.SetNodeSupply(it.first, it.second);
            }
            int solveStatus = minCostFlow.SolveMaxFlowWithMinCost();
            cout << "Checked For: " << it->first << " :: " << it->second.passedFlow << endl;
            if (solveStatus == SimpleMinCostFlow::OPTIMAL) {
                double equipmentCost = this->calculateEquipmentCostOfNetwork(minCostFlow);
                if (equipmentCost < this->currentSolution.totalEquipmentCost && this->isNetworkHasPinnedEdges(minCostFlow)) { //if network does not have pinned edges then ignore it even if it has lower cost in phase 2
                    costCompFile << time(0) << "," << equipmentCost << ",l," << it->second.passedFlow << endl;
                    isFurtherOptimization = true;
                    this->edgesKept.erase(it->first);
                    if(onlyEdgesWithLeafNodes){
                        writeJSONOutput(minCostFlow, -1);
                    }else{
                        writeJSONOutput(minCostFlow, -2);
                    }

                    for(auto it_sl = this->currentSolution.solutionEdges.begin(); it_sl != this->currentSolution.solutionEdges.end(); ++it_sl){
                        if(it_sl->second.unitCost > 0){
                            this->edgesKept[it_sl->first] = it_sl->second;
                        }
                    }
                    sortedKeptEdgeList.clear();
                    break;
                } else {
                    costCompFile << time(0) << "," << equipmentCost << ",u," << it->second.passedFlow << endl;
                    this->edgesKept[it->first].isChecked = false;
                    this->currentSolution.solutionEdges[it->first].isChecked = false;
                }
            } else {
                cout << "No Optimal Solution Found. Error In Further Optimization " << endl;
            }
            cout << "******************" << endl;

        }

    }while(isFurtherOptimization);
    costCompFile.close();
}

int MCFRR::solveInIterations(unsigned long randomSeed) {
    //important seed: 7415451
    mt19937 randomEng;
    randomEng.seed(randomSeed);
    int iterCount = 0;
    struct timeval start, end;
    gettimeofday(&start, NULL);
//     for (auto const& x : this->edgesKept)
// {
//     std::cout << x.first  // string (key)
//               << ':'
//               << x.second.tail << "," << x.second.head// string's value
//               << std::endl ;
// }
//     exit(1);
    while(true){
        this->totalNetworkEdges = 0;
        SimpleMinCostFlow minCostFlow;
        unsigned long long int totalNodes = (unsigned long long int)this->rows * this->cols;
        for(long i = 0; i < this->networkNodes.size(); i++) {
            for (long j = 0; j < this->networkNodes.size(); j++) {
                if(this->pinnedEdges.count(to_string(i)+","+to_string(j)) > 0){
                    cout << i << ":" << j << endl;
                    minCostFlow.AddArcWithCapacityAndUnitCost(i,j,INT_MAX, 0);
                }else if(getBit(this->networkNodes[i] * totalNodes + this->networkNodes[j])){
                    if(i == j){
                        continue;
                    }
                    this->totalNetworkEdges++;
                    double distance = this->calculateDistance(this->allCoordinates[ this->networkNodes[i] ], this->allCoordinates[ this->networkNodes[j] ]);
                    double unitCost = this->getEdgeUnitCost(distance);
                    if(this->edgesKept.count(to_string(i)+","+to_string(j)) > 0){
                        unitCost = 0;
                    }else if(this->currentSolution.solutionEdges.count(to_string(i)+","+to_string(j)) > 0){
                        Edge existingEdge = this->currentSolution.solutionEdges[to_string(i)+","+to_string(j)]; //solutionEdgeMap Hold the solution of previous iteration.
                        int probPercent = (int)ceil( ( (double)existingEdge.passedFlow / (double)existingEdge.capacity ) * 100.0 );
                        uniform_int_distribution<> distr(0, 100); // define the rang
                        int randomProb = distr(randomEng);
                        if(probPercent > randomProb){
                            this->edgesKept[to_string(i)+","+to_string(j)] = existingEdge;
                            unitCost = 0;
                        }
                    }
                    minCostFlow.AddArcWithCapacityAndUnitCost(i,j,INT_MAX, unitCost);
                }
            }
        }
        for (auto &it : this->sourceMap) {
            minCostFlow.SetNodeSupply(it.first, it.second);
        }
        for (auto &it : this->sinksMap) {
            minCostFlow.SetNodeSupply(it.first, it.second);
        }

        cout << "Kept Edges: " << this->edgesKept.size() << " / " << this->totalNetworkEdges << " so far" << endl;
        cout << "Start Solving For Iteration: " << iterCount << endl;
        int solveStatus = minCostFlow.SolveMaxFlowWithMinCost();
        if (solveStatus == SimpleMinCostFlow::OPTIMAL){
            cout << "Optimal Solution Found For Iter: " << iterCount << endl;
            writeJSONOutput(minCostFlow, iterCount);
            if(this->currentSolution.totalPinnedCost == 0){
                break;
            }
        }else{
            cout << "No Optimal Solution Found For Iter: " << iterCount << endl;
        }
        cout << "******************" << endl;
        iterCount++;
    }
    gettimeofday(&end, NULL);
    cout << "Total Time For all Iterations in Run: " << this->runIndex << " is: " << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000 << " Seconds" << endl;
    return iterCount;
}

MCFRR::~MCFRR() {
    cout << "MCFRR Says: Good Bye!" << endl;
    free(this->visibilityMatrix);
    delete [] this->allCoordinates;
    delete [] this->equipmentList;
}

const Solution &MCFRR::getPreviousSolution() const {
    return currentSolution;
}

void MCFRR::setPreviousSolution(const Solution &previousSolution) {
    MCFRR::currentSolution = previousSolution;
}


