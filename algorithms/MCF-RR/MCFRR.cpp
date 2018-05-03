#include "MCFRR.h"

MCF::MCF(char *outputDir, char* removalAlgorithm, char* equipmentFilePath, bool applyRemoval, double edgeDistanceScaling) {
    this->mainOutputDir = outputDir;
    this->removalAlgorithm = removalAlgorithm;
    this->equipmentFilePath = equipmentFilePath;
    this->equipmentList = new Equipment[EQUIPMENT_LENGTH];
    this->totalNetworkEdges = 0;
    this->applyRemoval = applyRemoval;
    this->edgeDistanceScaling = edgeDistanceScaling;
    initializePreliminaryData();
}

short MCF::getBit(unsigned long long int k) {
    return ((this->visibilityMatrix[k / 8] & (1 << (k % 8))) != 0);
}

double MCF::deg2rad(double deg) {
    return deg * (M_PI/180);
}

double MCF::calculateDistance(GPSPoint p1, GPSPoint p2) {
    double R = 6371; // Radius of the earth in km
    double dLat = deg2rad(p2.lat-p1.lat);  // deg2rad below
    double dLon = deg2rad(p2.lng-p1.lng);
    double a = sin(dLat/2) * sin(dLat/2) + cos(deg2rad(p1.lat)) * cos(deg2rad(p2.lat)) * sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return (R * c) * this->edgeDistanceScaling; // Distance in km
}




Equipment MCF::getBestDevice(GPSPoint p1, GPSPoint p2, int capacityToCompare) {
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

void MCF::initializePreliminaryData() {
    string line;
    char filePathBuffer[100];

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
    //initilizing memories
    this->allCoordinates = new GPSPoint[this->rows * this->cols];
    this->nonRemovedPoints = new bool[this->rows * this->cols]();
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

    //reading Non Removal Nodes
    int nodeIndex = -1;
    filePathBuffer[0] = '\0';
    sprintf(filePathBuffer, "%s/%s.txt", this->mainOutputDir, this->removalAlgorithm);
    ifstream fin(filePathBuffer);
    while(fin >> nodeIndex){
        this->nonRemovedPoints[nodeIndex] = true;
    }
    fin.close();

    //reading input json
    filePathBuffer[0] = '\0';
    sprintf(filePathBuffer, "%s/input.json", this->mainOutputDir);
    ifstream inputJsonHandel(filePathBuffer);
    inputJsonHandel >> this->inputJson;

    if (!this->inputJson.empty()){
        int maxCapacity = INT_MIN;
        for (json::iterator it = inputJson["nodes"].begin(); it != inputJson["nodes"].end(); ++it) {
            if (it[0]["nodeProperty"]["type"] == "source")
            {
                int source = int(it[0]["node"]) - 1;
                this->sourceMap[source] = int(it[0]["nodeProperty"]["capacity"]);
            } else {
                int sinkNode = int(it[0]["node"]) - 1;
                int capacity = int(it[0]["nodeProperty"]["capacity"]) * -1;
                this->sinksMap[sinkNode] += capacity;
                if(capacity > maxCapacity){
                    this->maximumCapacity = capacity;
                    maxCapacity = capacity;
                }
            }
        }
    }
    inputJsonHandel.close();


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
vector<Solution>& MCF::getAllSolutions() {
    return this->allSolutions;
}


void MCF::populateModel(double edgeDistanceLimit) {
    cout << "Start populating the modal" << endl;
    unsigned long long int totalNodes = (unsigned long long int)this->rows * this->cols;
    for(int i = 0; i < totalNodes; i++) {
        for (int j = 0; j < totalNodes; j++) {
            if(this->applyRemoval && !(this->nonRemovedPoints[i] && this->nonRemovedPoints[j]) ){
                continue;
            }
            double distance = this->calculateDistance(this->allCoordinates[i], this->allCoordinates[j]);
            if(edgeDistanceLimit > 0 && distance > edgeDistanceLimit){
                continue;
            }
            if(this->getBit(i * totalNodes + j)){
                Equipment equipment = this->getBestDevice( this->allCoordinates[i], this->allCoordinates[j], this->maximumCapacity );
                if(equipment.id == -1){
                    continue;
                }
                double unitCost =  this->getEdgeCost(distance);//ceil((double)equipment.totalPairCost / (double)equipment.throughput);
                this->simpleMinCostFlow.AddArcWithCapacityAndUnitCost(i,j,equipment.throughput, unitCost);
                this->totalNetworkEdges++;
            }
        }
    }
    cout << "Total Edges: " << this->totalNetworkEdges << endl;
    for (auto &it : this->sourceMap) {
        this->simpleMinCostFlow.SetNodeSupply(it.first, it.second);
    }
    for (auto &it : this->sinksMap) {
        this->simpleMinCostFlow.SetNodeSupply(it.first, it.second);
    }

}

double MCF::calculateEquipmentCostOnly(const SimpleMinCostFlow &minCostFlow) {
    double totalEquipmentCost = 0.0;
    for(int i = 0; i < minCostFlow.NumArcs(); i++) {
        if (minCostFlow.Flow(i) > 1) {
            Equipment equipment = this->getBestDevice(this->allCoordinates[minCostFlow.Tail(i)], this->allCoordinates[ minCostFlow.Head(i)], minCostFlow.Flow(i));
            totalEquipmentCost += equipment.totalPairCost;
        }
    }
    return totalEquipmentCost;
}

void MCF::writeJSONOutput(const SimpleMinCostFlow &minCostFlow, int iter) {
    char nameBuffer[300] = {'\0'};
    json output;
    output["bandwidth"] = 12323232;
    output["time"] = time(0);
    double originalCost = 0.0;
    double totalCost = 0.0;
    double totalEquipmentCost = 0.0;
    int outputCounter = 0;
    Solution solution;
    map<int, int> nodes;
    for(int i = 0; i < minCostFlow.NumArcs(); i++){
        if(minCostFlow.Flow(i) > 0){
            Edge edge;
            Equipment equipment = this->getBestDevice(this->allCoordinates[minCostFlow.Tail(i)], this->allCoordinates[ minCostFlow.Head(i)], minCostFlow.Flow(i));
            output["edges"][outputCounter]["nodes"] = json::array({minCostFlow.Tail(i), minCostFlow.Head(i)});
            output["edges"][outputCounter]["edgeProperty"]["unitCost"] = minCostFlow.UnitCost(i);
            output["edges"][outputCounter]["edgeProperty"]["bandwidth"] = equipment.throughput;//minCostFlow.Capacity(i);
            output["edges"][outputCounter]["edgeProperty"]["frequency"] = "2.4 GHz";
            output["edges"][outputCounter]["edgeProperty"]["flowPassed"] = minCostFlow.Flow(i);
            double distance = calculateDistance(this->allCoordinates[minCostFlow.Tail(i)], this->allCoordinates[ minCostFlow.Head(i)]);
            double originalUnitCost = this->getEdgeCost(distance);
            output["edges"][outputCounter]["edgeProperty"]["deviceCost"] = equipment.totalPairCost;
            nameBuffer[0] = '\0';
            sprintf(nameBuffer, "%.2f Km", distance);
            output["edges"][outputCounter]["edgeProperty"]["length"] = nameBuffer;
            nodes[minCostFlow.Head(i)] = minCostFlow.Supply(minCostFlow.Head(i));
            nodes[minCostFlow.Tail(i)] = minCostFlow.Supply(minCostFlow.Tail(i));
            edge.tail = minCostFlow.Tail(i);
            edge.head = minCostFlow.Head(i);
            edge.capacity = equipment.throughput;//minCostFlow.Capacity(i);
            edge.passedFlow = minCostFlow.Flow(i);
            edge.edgeLength = distance;
            edge.unitCost = minCostFlow.UnitCost(i);
            edge.equipment = equipment;
            solution.solutionEdges[to_string(edge.tail)+","+to_string(edge.head)] = edge;
            if(this->edgesKept.count(to_string(edge.tail)+","+to_string(edge.head)) > 0){
                this->edgesKept[to_string(edge.tail)+","+to_string(edge.head)].passedFlow = minCostFlow.Flow(i);
            }
            originalCost += originalUnitCost;
            totalCost +=  minCostFlow.UnitCost(i);
            totalEquipmentCost += equipment.totalPairCost;
            outputCounter++;
        }
    }
    solution.solutionNumber = iter;
    solution.totalEquipmentcost = round(totalEquipmentCost);
    solution.totalOriginalCost = round(originalCost);
    solution.totalPinnedCost = totalCost;
    this->allSolutions.push_back(solution);
    outputCounter = 0;
    for(auto ii = nodes.begin(); ii != nodes.end(); ++ii) {
        output["nodes"][outputCounter]["node"] = (*ii).first;
        output["nodes"][outputCounter]["nodeProperty"]["capacity"] = (*ii).second;
        output["nodes"][outputCounter]["nodeProperty"]["mountingHeight"] = 1;
        if ((*ii).second < 0)
        {
            output["nodes"][outputCounter]["nodeProperty"]["type"] = "sink";
        } else if ((*ii).second > 0)
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
    sprintf(nameBuffer, "%s/MCF/%s_%d/output_%d.json",this->mainOutputDir, this->removalAlgorithm, this->runIndex, iter);
    ofstream outputJsonFile(nameBuffer);
    outputJsonFile << setw(4) << output;
    outputJsonFile.close();
}

double MCF::getEdgeCost(double edgeLength) {
    return ( ( 0.1258 * pow(edgeLength, 2) ) + (6.7747 * edgeLength) + 63.429 );
}

void MCF::clearMetaData() {
    this->allSolutions.clear();
    this->edgesKept.clear();
}

bool MCF::isEdgeWithLeafNode(string edge) {
    stringstream ss(edge);
    vector<long> nodesInEdge;
    while(ss.good()){
        string subString;
        getline( ss, subString, ',' );
        nodesInEdge.push_back( stol(subString) );
    }
    for(auto eIt = this->allSolutions[0].solutionEdges.begin(); eIt !=  this->allSolutions[0].solutionEdges.end(); ++eIt){
        if(nodesInEdge[1] == eIt->second.tail){
            return false;
        }
    }
    return true;
}

void MCF::optimizeMinSolution(int iterCount) {
    cout << "Start Further Optimizing Min Solution" << endl;
    bool isFurtherOptimization;
    char filePathBuffer[200] = {'\0'};
    sprintf(filePathBuffer, "%s/costCompFileOneUnit.txt", this->mainOutputDir);
    ofstream costCompFile(filePathBuffer);
    do{
        isFurtherOptimization = false;
        for(auto it = this->edgesKept.begin(); it != this->edgesKept.end(); ++it) {
            if (!this->isEdgeWithLeafNode(it->first)) {
               cout << "This is skipped in part-1 of phase 2: " << it->first << "::" << it->second.passedFlow << endl;
                continue;
            }
            it->second.isChecked = true;
            this->allSolutions[0].solutionEdges[it->first].isChecked = true;
            SimpleMinCostFlow minCostFlow;
            unsigned long long int totalNodes = (unsigned long long int) this->rows * this->cols;
            for (long i = 0; i < totalNodes; i++) {
                for (long j = 0; j < totalNodes; j++) {
                    if (this->applyRemoval && !(this->nonRemovedPoints[i] && this->nonRemovedPoints[j])) {
                        continue;
                    }
                    if (getBit(i * totalNodes + j)) {
                        double distance = this->calculateDistance(this->allCoordinates[i], this->allCoordinates[j]);
                        Equipment equipment = this->getBestDevice(this->allCoordinates[i], this->allCoordinates[j], this->maximumCapacity);
                        if (equipment.id == -1) {
                            continue;
                        }
                        double unitCost = this->getEdgeCost(distance); //ceil((double)equipment.totalPairCost / (double)equipment.throughput);
                        if (this->edgesKept.count(to_string(i) + "," + to_string(j)) > 0 && !this->edgesKept[to_string(i) + "," + to_string(j)].isChecked) {
                            unitCost = 0;
                        } else if (this->allSolutions[0].solutionEdges.count(to_string(i) + "," + to_string(j)) > 0 && !this->allSolutions[0].solutionEdges[to_string(i) + "," + to_string(j)].isChecked) {
                            Edge existingEdge = this->allSolutions[0].solutionEdges[to_string(i) + "," + to_string(j)]; //solutionEdgeMap Hold the solution of previous iteration.
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
            if (solveStatus == SimpleMinCostFlow::OPTIMAL) {
                double equipmentCost = this->calculateEquipmentCostOnly(minCostFlow);
                if (equipmentCost < this->allSolutions[0].totalEquipmentcost) {
                    costCompFile << time(0) << "," << equipmentCost << ",l" << endl;
                    isFurtherOptimization = true;
                    this->edgesKept.erase(it->first);
                    this->allSolutions.clear();
                    writeJSONOutput(minCostFlow, iterCount);
                    for(auto it_sl = this->allSolutions[0].solutionEdges.begin(); it_sl != this->allSolutions[0].solutionEdges.end(); ++it_sl){
                        if(it_sl->second.unitCost > 0){
                            this->edgesKept[it_sl->first] = it_sl->second;
                        }
                    }
                    break;
                } else {
                    costCompFile << time(0) << "," << equipmentCost << ",u" << endl;
                    this->edgesKept[it->first].isChecked = false;
                    this->allSolutions[0].solutionEdges[it->first].isChecked = false;
                }
            } else {
                cout << "No Optimal Solution Found For Iter: " << iterCount << endl;
            }
            cout << "******************" << endl;

        }

    }while(isFurtherOptimization);
    costCompFile.close();
}

void MCF::furtherOptimizeMinSolution(int iterCount) {
    cout << "Start Further Optimizing Min Solution" << endl;
    bool isFurtherOptimization;
    char filePathBuffer[200] = {'\0'};
    sprintf(filePathBuffer, "%s/costCompFileMultipleUnit.txt", this->mainOutputDir);
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
        for (auto it = sortedKeptEdgeList.begin(); it != sortedKeptEdgeList.end(); ++it) {
            // std::cout << '{' << it->first << "," << it->second.passedFlow << '}' << endl;
        }
        for(auto it = sortedKeptEdgeList.begin(); it != sortedKeptEdgeList.end(); ++it) {

            if (this->isEdgeWithLeafNode(it->first)) {
               cout << "This edge is skipped in part-2 of phase 2: " << it->first << "::" << it->second.passedFlow << endl;
                continue;
            }
            //it->second.isChecked = true;
            this->edgesKept[it->first].isChecked = true;
            this->allSolutions[0].solutionEdges[it->first].isChecked = true;
            SimpleMinCostFlow minCostFlow;
            unsigned long long int totalNodes = (unsigned long long int) this->rows * this->cols;
            for (long i = 0; i < totalNodes; i++) {
                for (long j = 0; j < totalNodes; j++) {

                    if (this->applyRemoval && !(this->nonRemovedPoints[i] && this->nonRemovedPoints[j])) {
                        continue;
                    }
                    if (getBit(i * totalNodes + j)) {
                        double distance = this->calculateDistance(this->allCoordinates[i], this->allCoordinates[j]);
                        Equipment equipment = this->getBestDevice(this->allCoordinates[i], this->allCoordinates[j], this->maximumCapacity);
                        if (equipment.id == -1) {
                            continue;
                        }
                        double unitCost = this->getEdgeCost(distance); //ceil((double)equipment.totalPairCost / (double)equipment.throughput);
                        if (this->edgesKept.count(to_string(i) + "," + to_string(j)) > 0 && !this->edgesKept[to_string(i) + "," + to_string(j)].isChecked) {
                            unitCost = 0;
                        } else if (this->allSolutions[0].solutionEdges.count(to_string(i) + "," + to_string(j)) > 0 && !this->allSolutions[0].solutionEdges[to_string(i) + "," + to_string(j)].isChecked) {
                            Edge existingEdge = this->allSolutions[0].solutionEdges[to_string(i) + "," + to_string(j)]; //solutionEdgeMap Hold the solution of previous iteration.
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
                double equipmentCost = this->calculateEquipmentCostOnly(minCostFlow);
                if (equipmentCost < this->allSolutions[0].totalEquipmentcost) {
                    costCompFile << time(0) << "," << equipmentCost << ",l," << it->second.passedFlow << endl;
                    isFurtherOptimization = true;
                    this->edgesKept.erase(it->first);
                    this->allSolutions.clear();
                    writeJSONOutput(minCostFlow, iterCount);
                    for(auto it_sl = this->allSolutions[0].solutionEdges.begin(); it_sl != this->allSolutions[0].solutionEdges.end(); ++it_sl){
                        if(it_sl->second.unitCost > 0){
                            this->edgesKept[it_sl->first] = it_sl->second;
                        }
                    }
                    sortedKeptEdgeList.clear();
                    break;
                } else {
                    costCompFile << time(0) << "," << equipmentCost << ",u," << it->second.passedFlow << endl;
                    this->edgesKept[it->first].isChecked = false;
                    this->allSolutions[0].solutionEdges[it->first].isChecked = false;
                }
            } else {
                cout << "No Optimal Solution Found For Iter: " << iterCount << endl;
            }
            cout << "******************" << endl;

        }

    }while(isFurtherOptimization);
    costCompFile.close();
}



int MCF::solveInIterations( int randomSeed) {
    //important seed: 7415451
    mt19937 randomEng;
    randomEng.seed(randomSeed);
    int iterCount = 0;
    while(true){
        Solution previousSolution;
        if(iterCount > 0){
            previousSolution = this->allSolutions[iterCount-1];
        }
        struct timeval start, end;
        SimpleMinCostFlow minCostFlow;
        unsigned long long int totalNodes = (unsigned long long int)this->rows * this->cols;
        for(long i = 0; i < totalNodes; i++) {
            for (long j = 0; j < totalNodes; j++) {
                if(this->applyRemoval && !(this->nonRemovedPoints[i] && this->nonRemovedPoints[j]) ){
                    continue;
                }
                if(getBit(i * totalNodes + j)){
                    double distance = this->calculateDistance(this->allCoordinates[i], this->allCoordinates[j]);
                    Equipment equipment = this->getBestDevice( this->allCoordinates[i], this->allCoordinates[j], this->maximumCapacity );
                    if(equipment.id == -1){
                        continue;
                    }
                    double unitCost = this->getEdgeCost(distance); //ceil((double)equipment.totalPairCost / (double)equipment.throughput);
                    if(this->edgesKept.count(to_string(i)+","+to_string(j)) > 0){
                        unitCost = 0;
                    }else if(previousSolution.solutionEdges.count(to_string(i)+","+to_string(j)) > 0){
                        Edge existingEdge = previousSolution.solutionEdges[to_string(i)+","+to_string(j)]; //solutionEdgeMap Hold the solution of previous iteration.
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
        cout << "Kept Edges: " << this->edgesKept.size() << " so far" << endl;
        cout << "Start Solving For Iteration: " << iterCount << endl;
        gettimeofday(&start, NULL);
        int solveStatus = minCostFlow.SolveMaxFlowWithMinCost();
        gettimeofday(&end, NULL);
        if (solveStatus == SimpleMinCostFlow::OPTIMAL){
            cout << "Optimal Solution Found For Iter: " << iterCount << endl;
            writeJSONOutput(minCostFlow, iterCount);
            if(this->allSolutions[ this->allSolutions.size()-1 ].totalPinnedCost == 0){
                break;
            }
            cout << "Total Time For Iter " << iterCount << " is: " << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000 << " Seconds" << endl;
        }else{
            cout << "No Optimal Solution Found For Iter: " << iterCount << endl;
        }
        cout << "******************" << endl;
        iterCount++;
    }

    return iterCount;

}

void MCF::solve() {
    struct timeval start, end;
    cout << "Start Solving" << endl;
    gettimeofday(&start, NULL);
    int solveStatus = this->simpleMinCostFlow.SolveMaxFlowWithMinCost();
    gettimeofday(&end, NULL);
    if (solveStatus == SimpleMinCostFlow::OPTIMAL){
        cout << "Optimal Solution Found" << endl;
        writeJSONOutput(this->simpleMinCostFlow, 0);
        cout << "Total Time: " << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000 << " Seconds" << endl;
        cout << this->simpleMinCostFlow.OptimalCost() << "," << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) <<","<< this->totalNetworkEdges << endl;
    }else{
        cout << "No Optimal Solution Found" << endl;
    }
}

MCF::~MCF() {
    cout << "Call Dest." << endl;
    free(this->visibilityMatrix);
    delete [] this->nonRemovedPoints;
    delete [] this->allCoordinates;
    delete [] this->equipmentList;
}

void MCF::setRunIndex(int runIndex) {
    MCF::runIndex = runIndex;
}

const map<string, Edge> &MCF::getEdgesKept() const {
    return edgesKept;
}

void MCF::setEdgesKept(const map<string, Edge> &edgesKept) {
    MCF::edgesKept = edgesKept;
}

void MCF::pushSolution(Solution solution) {
    this->allSolutions.push_back(solution);
}
