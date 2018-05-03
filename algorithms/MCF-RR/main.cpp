#include "MCFRR.cpp"

int main(int argc, char *argv[]) {
    cout << "Starting MCF" << endl;
    /*
    1. : main output dir
    2. : selected points file name
    3. : equipment file name
    4. : is removal apply
    5. : Scaling of edge distance i.e. multiple each edge distance with this number to make the map virtually bigger.
    */
    Solution minSolution;
    struct timeval start, end;
    minSolution.totalEquipmentcost = DBL_MAX;
    map<string, Edge> edgesKeptInMinSolution;
    int minRun = 0, totalRuns = stoi(argv[5]);
    MCF mcf(argv[1], argv[2], argv[3], static_cast<bool>(stoi(argv[4])), 1);
    gettimeofday(&start, NULL);
    char filePathBuff[200] = {'\0'};
    sprintf(filePathBuff, "%s/MCFOutput.txt", argv[1]);
    ofstream outputFile(filePathBuff);
    for(int i = 0; i < totalRuns; i++){
        mcf.setRunIndex(i+1);
        //7415451
        mcf.solveInIterations(stoi(argv[6])+(i+1)*10);
        Solution lastSolution = mcf.getAllSolutions()[ mcf.getAllSolutions().size()-1 ];
        cout << "Equipment Cost Last: " << lastSolution.totalEquipmentcost << endl;
        if(lastSolution.totalEquipmentcost < minSolution.totalEquipmentcost){
            minSolution = lastSolution;
            edgesKeptInMinSolution = mcf.getEdgesKept();
            minRun = i;
        }
        mcf.clearMetaData();
    }
    cout << "Min Solution: " << minSolution.totalEquipmentcost << " found in run: " << minRun+1 << endl;
    outputFile << "Seed of this run: " << argv[6] << endl;
    outputFile << "Min Solution: " << minSolution.totalEquipmentcost << " found in run: " << minRun+1 << endl;
    filePathBuff[0] = '\0';
    sprintf(filePathBuff, "%s/MCF/noRemoval_%d", argv[1], minRun+1);
    outputFile << filePathBuff << endl;
    mcf.setEdgesKept(edgesKeptInMinSolution);
    mcf.pushSolution(minSolution);
    mcf.setRunIndex(minRun+1);
    mcf.optimizeMinSolution(9635);
    mcf.furtherOptimizeMinSolution(7415);
    gettimeofday(&end, NULL);
    outputFile << "Total Equipment Cost (FINAL):"<< mcf.getAllSolutions()[ 0 ].totalEquipmentcost << endl;
    cout << "Total Time: " << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000 << " Seconds" << endl;
    outputFile << "Total Time: " << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000 << " Seconds" << endl;
    outputFile.close();
    return 0;
}
