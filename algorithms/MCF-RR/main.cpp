#include "MCFRR.cpp"




int main(int argc, char *argv[]) {
    cout << "Starting MCF" << endl;
    /*
    1. : main output dir
    2. : selected points file name
    3. : equipment file name
    4. : Scaling of edge distance i.e. multiple each edge distance with this number to make the map virtually bigger.
    5. : Total Runs For MCFRR
    6. : Initial Seed For The Process
    */
    Solution minSolution;
    struct timeval start, end;
    minSolution.totalEquipmentCost = DBL_MAX;
    map<string, Edge> edgesKeptInMinSolution;
    int minRun = 0, totalRuns = stoi(argv[5]);
    MCFRR mcf(argv[1], argv[2], argv[3], stoi(argv[4]));
    gettimeofday(&start, NULL);
    char filePathBuff[200] = {'\0'};
    sprintf(filePathBuff, "%s/MCFOutput.txt", argv[1]);
    ofstream outputFile(filePathBuff);
    for(int i = 0; i < totalRuns; i++){
        mcf.setRunIndex(i+1);
        //7415451
        mcf.solveInIterations((unsigned long)stoi(argv[6])+(i+1)*10);
        Solution lastSolution = mcf.getPreviousSolution();
        cout << "Equipment Cost Last: " << lastSolution.totalEquipmentCost << endl;
        if(lastSolution.totalEquipmentCost < minSolution.totalEquipmentCost){
            minSolution = lastSolution;
            edgesKeptInMinSolution = mcf.getEdgesKept();
            minRun = i;
        }
        mcf.clearMetaData();
    }
    cout << "Min Solution: " << minSolution.totalEquipmentCost << " found in run: " << minRun+1 << endl;
    outputFile << "Seed of this run: " << argv[6] << endl;
    outputFile << "Min Solution: " << minSolution.totalEquipmentCost << " found in run: " << minRun+1 << endl;
    filePathBuff[0] = '\0';
    sprintf(filePathBuff, "%s/MCF/noRemoval_%d", argv[1], minRun+1);
    outputFile << filePathBuff << endl;
    mcf.setEdgesKept(edgesKeptInMinSolution);
    mcf.setPreviousSolution(minSolution);
    mcf.setRunIndex(minRun+1);
    cout << "Starting Phase 2 => Part-1" << endl;
    mcf.doFurtherOptimization(true); //first edges with leaf nodes.
    cout << "Starting Phase 2 => Part-2" << endl;
    mcf.doFurtherOptimization(false); // then all other edges sorted by flow passed.
    gettimeofday(&end, NULL);
    outputFile << "Total Equipment Cost (FINAL):"<< mcf.getPreviousSolution().totalEquipmentCost << endl;
    cout << "Total Time: " << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000 << " Seconds" << endl;
    outputFile << "Total Time: " << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000 << " Seconds" << endl;
    outputFile.close();
    return 0;
}