//
// Created by cuda on 3/6/18.
//

#include <sys/stat.h>
#include <sys/time.h>
#include "CPLEX.h"

CPLEX::CPLEX(char *outputDir, char* removalAlgorithm, char* equipmentFilePath, double timeLimit, bool applyRemoval, double edgeDistanceScaling) {
    this->mainOutputDir = outputDir;
    this->removalAlgorithm = removalAlgorithm;
    this->equipmentFilePath = equipmentFilePath;
    this->model = IloModel(this->env);
    this->numVarArray = IloNumVarArray(this->env);
    this->boolVarArray = IloBoolVarArray(this->env);
    this->con = IloRangeArray(this->env);
    this->equipmentList = new Equipment[EQUIPMENT_LENGTH];
    this->cplexTimeLimit = timeLimit;
    this->totalNetworkEdges = 0;
    this->applyRemoval = applyRemoval;
    this->edgeDistanceScaling = edgeDistanceScaling;
    initializePreliminaryData();
}

short CPLEX::getBit(unsigned long long int k) {
    return ((this->visibilityMatrix[k / 8] & (1 << (k % 8))) != 0);
}

double CPLEX::deg2rad(double deg) {
    return deg * (M_PI/180);
}


double CPLEX::calculateDistance(GPSPoint p1, GPSPoint p2) {
    double R = 6371; // Radius of the earth in km
    double dLat = deg2rad(p2.lat-p1.lat);  // deg2rad below
    double dLon = deg2rad(p2.lng-p1.lng);
    double a = sin(dLat/2) * sin(dLat/2) + cos(deg2rad(p1.lat)) * cos(deg2rad(p2.lat)) * sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return (R * c); // Distance in km
}

Equipment CPLEX::getBestDevice(GPSPoint p1, GPSPoint p2, int capacityToCompare) {
    double distance = calculateDistance(p1,p2) * this->edgeDistanceScaling;
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

void CPLEX::initializePreliminaryData() {
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
                int capacity = int(it[0]["nodeProperty"]["capacity"])*1;
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


void CPLEX::populateModel(double edgeDistanceLimit) {
    cout << "Start populating the modal" << endl;
    unsigned long long int totalNodes = (unsigned long long int)this->rows * this->cols;
    NodeEdgeRef *allNodeEdgeRefs = (NodeEdgeRef*) malloc(totalNodes * sizeof(NodeEdgeRef));

    for(int i = 0; i < totalNodes; i++){
        allNodeEdgeRefs[i].outGoing = IloNumVarArray(env);
        allNodeEdgeRefs[i].inComing = IloNumVarArray(env);
    }
    IloEnv env = this->model.getEnv();
    IloNumVar var;
    IloBoolVar bVar;
    IloExpr objExpr(env);
    IloExpr capacityConstExpr(env);
    IloExpr conservativeConstExpr(env);
    char tempBuffer[30];
    cout << "The Maximum Capacity: " << this->maximumCapacity << endl;
    for(unsigned long long int i = 0; i < totalNodes; i++){
        for(unsigned long long int j = 0; j < totalNodes; j++){
            if(this->applyRemoval && !(this->nonRemovedPoints[i] && this->nonRemovedPoints[j]) ){
                continue;
            }
            double distance = this->calculateDistance(this->allCoordinates[i], this->allCoordinates[j]);
            if(edgeDistanceLimit > 0 && distance > edgeDistanceLimit){
                continue;
            }
            if(getBit(i * totalNodes + j)){

                Equipment equipment = this->getBestDevice( this->allCoordinates[i], this->allCoordinates[j], this->maximumCapacity );
                if(equipment.id == -1){
                    continue;
                }
                int capacity = equipment.throughput;
                int totalCost = equipment.totalPairCost;
                sprintf(tempBuffer, "x(%d,%d,%d)", i, j, 10);
                var = IloNumVar(env, 0.0,  capacity, tempBuffer);
                sprintf(tempBuffer, "y(%d,%d,%d)", i, j, 10);
                bVar = IloBoolVar(env, 0,  1, tempBuffer);
                this->numVarArray.add(var);
                this->boolVarArray.add(bVar);

                allNodeEdgeRefs[i].outGoing.add(var);
                allNodeEdgeRefs[j].inComing.add(var);

                //adding objectiveExpression
                objExpr += bVar * totalCost;

                //adding capacity constraints
                capacityConstExpr += var;
                capacityConstExpr += -(bVar * capacity);
                IloRange capacityConstraint(env, INT_MIN, capacityConstExpr, 0);
                this->con.add(capacityConstraint);
                capacityConstExpr.clear();
                this->totalNetworkEdges++;
            }
        }
    }

    cout << "Total Edges: " << this->totalNetworkEdges << endl;
    this->model.add(this->numVarArray);
    this->model.add(this->boolVarArray);
    this->model.add(IloMinimize(env, objExpr));

    for(int i = 0; i < totalNodes; i++){
        if(sourceMap.count(i) > 0){
            conservativeConstExpr.clear();
            for(int j = 0; j < allNodeEdgeRefs[i].outGoing.getSize(); j++){
                conservativeConstExpr += allNodeEdgeRefs[i].outGoing[j];
            }
            IloRange conservativeConstraint(env, 0, conservativeConstExpr, sourceMap[i]); //supply
            this->con.add(conservativeConstraint);
        }else if(this->sinksMap.count(i) > 0){
            conservativeConstExpr.clear();
            for(int j = 0; j < allNodeEdgeRefs[i].outGoing.getSize(); j++){
                conservativeConstExpr += -(allNodeEdgeRefs[i].outGoing[j]);
            }
            // cout << "IncomingSize: " << allNodeEdgeRefs[i].inComing.getSize() << endl;
            for(int j = 0; j < allNodeEdgeRefs[i].inComing.getSize(); j++){
                conservativeConstExpr += allNodeEdgeRefs[i].inComing[j];
            }
            //add in the modal if edges from this sink exist
            if(allNodeEdgeRefs[i].inComing.getSize() > 0){
                IloRange conservativeConstraint(env, sinksMap[i], conservativeConstExpr, sinksMap[i]); //demand
                this->con.add(conservativeConstraint);
            }
        }else{
            conservativeConstExpr.clear();
            for(int j = 0; j < allNodeEdgeRefs[i].outGoing.getSize(); j++){
                conservativeConstExpr += -(allNodeEdgeRefs[i].outGoing[j]);
            }
            for(int j = 0; j < allNodeEdgeRefs[i].inComing.getSize(); j++){
                conservativeConstExpr += allNodeEdgeRefs[i].inComing[j];
            }
            IloRange conservativeConstraint(env, 0, conservativeConstExpr, 0); //conserve constraint
            this->con.add(conservativeConstraint);
        }
    }
    this->model.add(this->con);
    this->cplex = IloCplex(this->model);

    //conversion
    // IloModel LpRelax(env);
    // LpRelax.add(this->model);
    // LpRelax.add(IloConversion(env, this->boolVarArray, ILOFLOAT));
    // this->cplex.extract(LpRelax);

    for(int i = 0; i < totalNodes; i++){
        allNodeEdgeRefs[i].outGoing.clear();
        allNodeEdgeRefs[i].inComing.clear();
    }
    free(allNodeEdgeRefs);
    free(this->visibilityMatrix);
    delete [] this->nonRemovedPoints;
}

void CPLEX::writeModel(const char *format) {
    char filePathBuffer[200] = {'\0'};
    sprintf(filePathBuffer, "%s/model.%s", this->mainOutputDir, format);
    cout << "Start Exporting into " << format << " format" << endl;
    this->cplex.exportModel(filePathBuffer);
}

vector<int> CPLEX::splitStringAndGetEdges(const string &s) {
    char buffer[20] = {'\0'};
    s.copy(buffer, s.length()-2, 2);
    string token;
    vector<int> tokens;
    for(int i = 0; buffer[i] != '\0'; i++){
        if(buffer[i] == ','){
            tokens.push_back(stoi(token));
            token = "";
            i++;
        }
        token += buffer[i];
    }
    return tokens;
}

void CPLEX::solve() {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    this->cplex.setParam(IloCplex::Param::RootAlgorithm, IloCplex::Network);
    this->cplex.setParam(IloCplex::Param::Preprocessing::Dual, 1);
    this->cplex.setParam(IloCplex::LongParam::IntSolLim, 1);
//    this->cplex.setParam(IloCplex::Param::Emphasis::Numerical, 1);
    this->cplex.setParam(IloCplex::TiLim, this->cplexTimeLimit);
    int solutionNumber = 1;
    int loopStartTime = time(0);
    IloNum totalCostOfNetwork = 0;
    char nameBuffer[300] = {'\0'};
    while(true){
        IloNumArray vals(env);
        //IloBoolArray boolVals(env);
        this->cplex.solve();
        env.out() << "Solution status = " << this->cplex.getStatus() << endl;
        env.out() << "Solution value  = " << this->cplex.getObjValue() << endl;
        IloAlgorithm::Status solStatus = this->cplex.getStatus();
        json output;
        int outputCounter = 0;
        totalCostOfNetwork = cplex.getObjValue();
        output["bandwidth"] = 12323232;
        output["time"] = time(0);
        map<int, int> allNodes;
        this->cplex.getValues(vals, this->numVarArray);
        //this->cplex.getValues(boolVals, this->boolVarArray);
        for(int i = 0; i < this->numVarArray.getSize(); i++){
            if(vals[i] > 1){
                vector<int> edgeTokens = splitStringAndGetEdges(this->numVarArray[i].getName());
                for(int i = 0; i < 2; i++){
                    if(sinksMap[edgeTokens[i]] > 0){
                        allNodes[edgeTokens[i]] = -sinksMap[edgeTokens[i]];
                    }else if(sourceMap[edgeTokens[i]] > 0){
                        allNodes[edgeTokens[i]] = sourceMap[edgeTokens[i]];
                    }else{
                        allNodes[edgeTokens[i]] = 0;
                    }
                }
                output["edges"][outputCounter]["nodes"] = json::array({edgeTokens[0], edgeTokens[1]});
                Equipment postSolveEquipment = getBestDevice(this->allCoordinates[edgeTokens[0]], this->allCoordinates[edgeTokens[1]], int(vals[i]));
                Equipment preSolveEquipment = getBestDevice(this->allCoordinates[edgeTokens[0]], this->allCoordinates[edgeTokens[1]], this->maximumCapacity);
                output["edges"][outputCounter]["edgeProperty"]["bandwidth"] = postSolveEquipment.throughput;
                output["edges"][outputCounter]["edgeProperty"]["frequency"] = "2.4 GHz";
                output["edges"][outputCounter]["edgeProperty"]["flowPassed"] = vals[i];
                //output["edges"][outputCounter]["edgeProperty"]["selectionVal"] = boolVals[i];
                totalCostOfNetwork -= preSolveEquipment.totalPairCost;
                totalCostOfNetwork += postSolveEquipment.totalPairCost;
                nameBuffer[0] = '\0';
                sprintf(nameBuffer, "$%d", postSolveEquipment.totalPairCost);
                output["edges"][outputCounter]["edgeProperty"]["edgeCost"] = nameBuffer;
                double distance = calculateDistance(this->allCoordinates[edgeTokens[0]], this->allCoordinates[edgeTokens[1]]);
                nameBuffer[0] = '\0';
                sprintf(nameBuffer, "%.2f Km", distance * this->edgeDistanceScaling);
                output["edges"][outputCounter]["edgeProperty"]["length"] = nameBuffer;
                outputCounter++;
            }
        }
        outputCounter = 0;
        for (auto &allNode : allNodes) {

            output["nodes"][outputCounter]["node"] = allNode.first;
            output["nodes"][outputCounter]["nodeProperty"]["capacity"] = allNode.second;
            output["nodes"][outputCounter]["nodeProperty"]["mountingHeight"] = 10;
            //cost of mounting height

            if (allNode.second < 0){
                output["nodes"][outputCounter]["nodeProperty"]["type"] = "sink";
            } else if (allNode.second > 0){
                output["nodes"][outputCounter]["nodeProperty"]["type"] = "source";
            }
            else {
                output["nodes"][outputCounter]["nodeProperty"]["type"] = "intermediate";
            }
            outputCounter++;
        }
        output["costs"] = totalCostOfNetwork;
        nameBuffer[0] = '\0';
        sprintf(nameBuffer, "%s/FCNF/", this->mainOutputDir);
        mkdir(nameBuffer, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        nameBuffer[0] = '\0';
        sprintf(nameBuffer, "%s/FCNF/%s", this->mainOutputDir, this->removalAlgorithm);
        mkdir(nameBuffer, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        nameBuffer[0] = '\0';
        sprintf(nameBuffer, "%s/FCNF/%s/solution_%d.json",this->mainOutputDir, this->removalAlgorithm, solutionNumber);
        // if(solutionNumber > 1){
            ofstream outputJsonFile(nameBuffer);
            outputJsonFile << setw(4) << output;
            outputJsonFile.close();
        // }
        solutionNumber++;
        if(solStatus == IloAlgorithm::Optimal || (time(0) - loopStartTime) >= this->cplexTimeLimit ){
            break;
        }
    }
    cout << "************" << endl;
    gettimeofday(&end, NULL);
    cout << "Total Time: " << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000 << " Seconds" << endl;
    cout << totalCostOfNetwork << "," << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) <<","<< this->totalNetworkEdges << endl;
    this->env.end();
}

CPLEX::~CPLEX() {
    delete [] this->allCoordinates;
    delete [] this->equipmentList;
}
