#include <iostream>
#include <fstream>
#include <climits>
#include <vector>
#include <cmath>
#include <queue>
#include <unistd.h>
#include <map>
#include <set>
#include <algorithm>
#include <ctime>
#include <list>
#include <cstring>

using namespace std;

#define BUCKET_SIZE 10
#define COMBINATION_THRESHOLD 20
int INSIDE_AND_OUTSIDE = 1;
double xDist = 0, yDist = 0;


struct Point{
    int row;
    int col;
    int elevation;
    int visibilityCount;
    set<int> localViewshed;
    Point(){
        this->visibilityCount = 0;
        this->elevation = 0;
    }

    bool operator== (const Point &c1) const {
        return (this->row == c1.row && this->col == c1.col);
    }

    bool operator< (const Point &c2) const
    {
        return visibilityCount < c2.visibilityCount;
    }

    bool operator<= (const Point &c2) const
    {
        return visibilityCount <= c2.visibilityCount;
    }
};

struct ViewshedCombination{
    vector<int> combination;
    vector<int> unionViewShed;
};

Point getPoint(int node, int width){
    Point p;
    p.row = node / width;
    p.col = node % width;
    return p;
}

struct ElevationBucket{
    int startRange;
    int endRange;
    vector<Point> allPointsInBucket;
};

string remove_multiple_spaces(string input){
    string output;
    int k = 0, i = 0;
    for(i = 0,k=0; i < input.length(); i++, k++){
        if(input[i] == ' ' && input[i+1] == ' '){
            k--;
            continue;
        }
        output += input[i];
    }
    output += '\0';
    return output;
}

string breakStringByspace(string input){
    bool spaceCome = false;
    string output;
    for(int i = 0; i < input.length(); i++){
        if(spaceCome){
            output += input[i];
        }
        if(input[i] == ' '){
            spaceCome =  true;
        }
    }
    output += '\0';
    return output;
}
void splitStringBySpace(string input, int * output, int &maximumElevation){
    string elev = "";
    int elevationindex = 0;
    int startCol = 0;
    // ignore space at beginning of line (happens when exporting file from GRASS)
    if (input[0] == ' ') {
        startCol = 1;
    }

    for(int i = startCol; i < input.length(); i++){
        if(input[i] == ' '){
            output[elevationindex] = atoi(elev.c_str());
            if(output[elevationindex] != 65535 && output[elevationindex] > maximumElevation){
                maximumElevation = output[elevationindex];
            }
            elevationindex++;
            elev = "";
            continue;
        }
        elev += input[i];
        if(i == input.length() - 1){
            output[elevationindex] = atoi(elev.c_str());
        }
    }
}

double getDistance(Point p1, Point p2){
    double x = p1.row * xDist - p2.row * xDist; //calculating number to square in next step
    double y = p1.col * yDist - p2.col * yDist;
    double dist;
    dist = pow(x, 2) + pow(y, 2);       //calculating Euclidean distance
    dist = sqrt(dist);
    return (dist) / 1000;

}

vector<Point> getNeighbours(int ** elevationMap, Point startPoint, int nRow, int nCol, bool** trackingMap){
    vector<Point> neighbours;
    Point top, left, right, bottom /*, topLeft, bottomLeft, topRight, bottomRight */;
    if(startPoint.row - 1 >= 0){ // else no top exist
        top.row = startPoint.row - 1;
        top.col = startPoint.col;
        top.elevation = elevationMap[top.row][top.col];
        if(!trackingMap[top.row][top.col] && top.elevation != 65535){
            neighbours.push_back(top);
        }
    }
    if(startPoint.col - 1 >= 0){ // else no left exist
        left.row = startPoint.row;
        left.col = startPoint.col - 1;
        left.elevation = elevationMap[left.row][left.col];
        if(!trackingMap[left.row][left.col] && left.elevation != 65535){
            neighbours.push_back(left);
        }

    }
    if(startPoint.row + 1 < nRow){ // else no bottom exist
        bottom.row = startPoint.row + 1;
        bottom.col = startPoint.col;
        bottom.elevation = elevationMap[bottom.row][bottom.col];
        if(!trackingMap[bottom.row][bottom.col] && bottom.elevation != 65535){
            neighbours.push_back(bottom);
        }
    }
    if(startPoint.col + 1 < nCol){ // else no right exist
        right.row = startPoint.row;
        right.col = startPoint.col + 1;
        right.elevation = elevationMap[right.row][right.col];
        if(!trackingMap[right.row][right.col] && right.elevation != 65535){
            neighbours.push_back(right);
        }
    }

    /*if(startPoint.row - 1 >= 0 && startPoint.col - 1 >= 0){ //else no top-left exist
        topLeft.row = startPoint.row - 1;
        topLeft.col = startPoint.col - 1;
        topLeft.elevation = elevationMap[topLeft.row][topLeft.col];
        if(!trackingMap[topLeft.row][topLeft.col] && topLeft.elevation != 65535){
            neighbours.push_back(topLeft);
        }
    }

    if(startPoint.row + 1 < nRow && startPoint.col - 1 >= 0){ //else no bottom left exist
        bottomLeft.row = startPoint.row + 1;
        bottomLeft.col = startPoint.col - 1;
        bottomLeft.elevation = elevationMap[bottomLeft.row][bottomLeft.col];
        if(!trackingMap[bottomLeft.row][bottomLeft.col] && bottomLeft.elevation != 65535){
            neighbours.push_back(bottomLeft);
        }
    }

    if(startPoint.row - 1 >= 0 && startPoint.col + 1 < nCol){ // else no top-right exist
        topRight.row = startPoint.row - 1;
        topRight.col = startPoint.col + 1;
        topRight.elevation = elevationMap[topRight.row][topRight.col];
        if(!trackingMap[topRight.row][topRight.col] && topRight.elevation != 65535){
            neighbours.push_back(topRight);
        }
    }

    if(startPoint.row + 1 < nRow && startPoint.col + 1 < nCol){
        bottomRight.row = startPoint.row + 1;
        bottomRight.col = startPoint.col + 1;
        bottomRight.elevation = elevationMap[bottomRight.row][bottomRight.col];
        if(!trackingMap[bottomRight.row][bottomRight.col] && bottomRight.elevation != 65535){
            neighbours.push_back(bottomRight);
        }
    }*/

    return neighbours;
}

Point getAvailablePoint(bool **trackingMap, int nRows, int nCols){
    Point p;
    p.row = -1;
    for(int i = 0; i < nRows; i++){
        for(int j = 0; j < nCols; j++){
            if(!trackingMap[i][j]){
                p.row = i;
                p.col = j;
                goto nowReturn;
            }
        }
    }
    nowReturn:    return p;
}

void readBinaryFile(unsigned char* &visibilityMatrix, long rows, char* binaryFile){
    //cout << "Start Reading Visibility Matrix" << endl;
    visibilityMatrix = (unsigned char *)calloc(ceil((float)(rows * rows) / 8) * sizeof(unsigned char), sizeof(unsigned char));
    FILE* fstr = fopen(binaryFile, "rb");
    fread(visibilityMatrix, sizeof(unsigned char), ceil((float)(rows * rows) / 8) * sizeof(unsigned char), fstr);
    fclose(fstr);
}
// ----- Get Bit -----
short getBit(unsigned char *A, unsigned long long int k) {
    return ((A[k / 8] & (1 << (k % 8))) != 0);
}

bool isInList(vector<int> mutedPoints, int p){
    for(int i = 0; i < mutedPoints.size(); i++){
        if(mutedPoints[i] == p){
            return true;
        }
    }
    return false;
}

int checkVisibility(bool *mutedArray, int nodeToCheck, int nRows, int nCols, unsigned char *visibilityMatrix, long visibilityMatrixNodes, bool *noDataValuesArray){
    int vCounter = 0;
    for(int i = 0; i < nRows; i++){
        for(int j = 0; j < nCols; j++){
            if(!mutedArray[i * nCols + j] && !noDataValuesArray[i * nCols + j] && (i * nCols + j) != nodeToCheck){
                if(getBit(visibilityMatrix, nodeToCheck * visibilityMatrixNodes + (i * nCols + j))){
                    vCounter++;
                }
            }
        }
    }
    return vCounter;
}

int main(int argc, char* argv[]) {
    ifstream fin;
    unsigned char *visibilityMatrix = NULL;
    cout << "Start Reading Elevation Data" << endl;
    fin.open(argv[3]);
    string line_ncols, line_nrows, line_xllcorner, line_yllcorner, line_dx, line_dy, line_nodata;
    getline(fin, line_ncols);
    getline(fin, line_nrows);
    getline(fin, line_xllcorner);
    getline(fin, line_yllcorner);
    getline(fin, line_dx);
    getline(fin, line_dy);
    getline(fin, line_nodata);
    line_ncols = remove_multiple_spaces(line_ncols);
    line_nrows = remove_multiple_spaces(line_nrows);
    if(line_nodata.find("NODATA_value") != string::npos){
        line_nodata = remove_multiple_spaces(line_nodata);
    }
    int nCols = atoi(breakStringByspace(line_ncols).c_str());
    int nRows = atoi(breakStringByspace(line_nrows).c_str());
    int noData;
    if(line_nodata.find("NODATA_value") == string::npos){
        noData = 65535;
    }else{
        noData = atoi(breakStringByspace(line_nodata).c_str());
    }
     cout << "Rows: " << nRows << ", Cols: " << nCols << endl;
    int **elevationMap = (int**) malloc(nRows * sizeof(int*));
    bool **trackingMap = (bool**) malloc(nRows * sizeof(bool*));
    for(int i = 0; i < nRows; i++){
        elevationMap[i] = (int*) malloc(nCols * sizeof(int));
        trackingMap[i] = (bool*) malloc(nCols * sizeof(bool));
    }
    int rowIndex = 0;
    int maximumElevation = INT_MIN;
    if(line_nodata.find("NODATA_value") == string::npos){
        splitStringBySpace(line_nodata, elevationMap[rowIndex], maximumElevation);
        rowIndex++;
    }
    while(getline(fin, line_ncols) && rowIndex < nRows){
        splitStringBySpace(line_ncols, elevationMap[rowIndex], maximumElevation);
        rowIndex++;
    }
    fin.close();
    long visibilityMatrixNodes = nRows * nCols;
    readBinaryFile(visibilityMatrix, visibilityMatrixNodes, argv[4]);
    /*Reading Complete*/
    for(int i = 0; i < nRows; i++){
        for(int j = 0; j < nCols; j++){
            //cout << elevationMap[i][j] << " ";
            trackingMap[i][j] = false;
        }
        // cout << endl;
    }
    // cout << "No Data: " << noData << endl;

    int totalBuckets = ceil(maximumElevation / BUCKET_SIZE);

    ElevationBucket *elevationBuckets = new ElevationBucket[totalBuckets];
    for(int i = 0,j=0; i < totalBuckets; i++,j+=BUCKET_SIZE){
        elevationBuckets[i].startRange = j;
        elevationBuckets[i].endRange = j+BUCKET_SIZE-1;
    }

    int totalNoDataValues = 0;
    ofstream noDataValuesHandler("../nodatavalues.txt");
    bool noDataValuesArray[visibilityMatrixNodes] = {false};
    for(int i = 0; i < nRows; i++){
        for(int j = 0; j < nCols; j++){
            if(elevationMap[i][j] == noData){
                totalNoDataValues++;
                trackingMap[i][j] = true;
                noDataValuesHandler << i * nCols + j << endl;
                noDataValuesArray[i * nCols + j] = true;
            }
        }
    }
    noDataValuesHandler.close();
    queue<Point> neighboursQueue;
    int iterIndex = 0;
    cout << "Start finding areas..." << endl;
    bool mutedArray[nRows * nCols];
    for(int i = 0; i < visibilityMatrixNodes; i++){
        mutedArray[i] = true;
    }
    list<Point> visibilityList;
    double removalPercent = stod(argv[1]);
    double visibilityRange = stod(argv[2]);
    int INSIDE_AND_OUTSIDE = stoi(argv[6]);
    xDist = stod(argv[7]);yDist = stod(argv[8]);
    ofstream mutedPointsFile(argv[5]);
    srand (356);
    int tempSize = 0,tempSize2 = 0;
    map<int, vector< Point > > allElevationBucketsMap;

    while(true){
        ElevationBucket elevationBucket;
        Point availablePoint = getAvailablePoint(trackingMap, nRows, nCols);
        if(availablePoint.row == -1){
            //cout << "Exit" << endl;
            break; // we cover every point
        }
        availablePoint.elevation = elevationMap[availablePoint.row][availablePoint.col];
        trackingMap[availablePoint.row][availablePoint.col] = true;
        elevationBucket.allPointsInBucket.push_back(availablePoint);
        neighboursQueue.push(availablePoint);
        //int desiredElevationBucketIndex = getElevationBucketIndex(elevationBuckets, availablePoint.elevation);
        int desiredElevationBucketIndex = availablePoint.elevation / BUCKET_SIZE;

        //allBuckets.push_back(elevationBuckets[desiredElevationBucketIndex]);
        while (!neighboursQueue.empty()){
            Point startingPoint = neighboursQueue.front();
            neighboursQueue.pop();
            vector<Point> neighboursOfStartingPoints = getNeighbours(elevationMap, startingPoint, nRows, nCols,trackingMap);
            for(int i = 0; i < neighboursOfStartingPoints.size(); i++){
                int bucketIndexOfNeighbour = neighboursOfStartingPoints[i].elevation / BUCKET_SIZE;
                if(bucketIndexOfNeighbour == desiredElevationBucketIndex){
                    trackingMap[neighboursOfStartingPoints[i].row][neighboursOfStartingPoints[i].col] = true;
                    elevationBucket.allPointsInBucket.push_back(neighboursOfStartingPoints[i]);
                    neighboursQueue.push(neighboursOfStartingPoints[i]);
                }
            }
        }
        // cout << "Watching Bucket: " << desiredElevationBucketIndex << ", " << availablePoint.elevation << endl;
        vector< Point > currentElevationBucketList = allElevationBucketsMap[desiredElevationBucketIndex];
        for(int e = 0; e < elevationBucket.allPointsInBucket.size(); e++){
            currentElevationBucketList.push_back(elevationBucket.allPointsInBucket[e]);
        }
        allElevationBucketsMap[desiredElevationBucketIndex] = currentElevationBucketList;
        cout << "Area Found" << endl;
        cout << "Size of area: " << elevationBucket.allPointsInBucket.size() << endl;
        vector<int> nonRemovedPoints;
        //if size is greater than threshold, just take remaining points randomly and calculate union of viewshed
        if(elevationBucket.allPointsInBucket.size() > COMBINATION_THRESHOLD){
            int remainingPoints = ceil( elevationBucket.allPointsInBucket.size() * (1 - (removalPercent / 100.0)) );
            int iterationToCheckMaxViewshed = 2;//ceil(elevationBucket.allPointsInBucket.size() / remainingPoints);
            long maximumUnion = INT_MIN;
            for(int l = 0; l < iterationToCheckMaxViewshed; l++){
                random_shuffle ( elevationBucket.allPointsInBucket.begin(), elevationBucket.allPointsInBucket.end());
                set<int> resultSet;
                vector<int> pointSet;
                for(int i = 0; i < remainingPoints; i++){
                    set<int> viewshedSet;
                    vector<int> resultVector;
                    Point refPoint = elevationBucket.allPointsInBucket[i];
                    long ithIndex = refPoint.row * nCols + refPoint.col;
                    switch (INSIDE_AND_OUTSIDE){
                        case 1:
                            for(long j = 0; j < visibilityMatrixNodes; j++){
                                double distance = getDistance(refPoint, getPoint(j, nCols));
                                if(ithIndex == j){
                                    continue;
                                }
                                if(visibilityRange != -1 && distance > visibilityRange){
                                    continue;
                                }
                                if(getBit(visibilityMatrix, ithIndex * visibilityMatrixNodes + j)){
                                    viewshedSet.insert(j);
                                }
                            }
                            break;
                        case 0:
                            for(long j = 0; j < elevationBucket.allPointsInBucket.size(); j++){
//                                cout << j << endl;
                                Point checkForVisibility = elevationBucket.allPointsInBucket[j];
                                if(refPoint == checkForVisibility){
                                    continue;
                                }
                                int jthIndex = checkForVisibility.row * nCols + checkForVisibility.col;
                                if(getBit(visibilityMatrix, ithIndex * visibilityMatrixNodes + jthIndex)){
                                    viewshedSet.insert(jthIndex);
                                }
                            }
                            break;
                    }
                    set_union (viewshedSet.begin(), viewshedSet.end(), resultSet.begin(), resultSet.end(), back_inserter(resultVector));
                    //copy items from vector to result set to again calculate the union
                    resultSet.clear();
                    for(int k = 0; k < resultVector.size(); k++){
                        resultSet.insert(resultVector[k]);
                    }
                    pointSet.push_back(ithIndex);
                }
                long currentSizeOfSet = resultSet.size();
                if(currentSizeOfSet > maximumUnion){
                    maximumUnion = resultSet.size();
                    nonRemovedPoints.clear();
                    copy(pointSet.begin(), pointSet.end(), back_inserter(nonRemovedPoints));
                }
            }
            for(int i = 0; i < nonRemovedPoints.size(); i++){
                mutedArray[nonRemovedPoints[i]] = false;
            }
//            cout << maximumUnion << endl;
            tempSize += nonRemovedPoints.size();
        }else{
            int N = elevationBucket.allPointsInBucket.size();
            int K = ceil(N * (1 - (removalPercent / 100.0)));
            long maximumUnion = INT_MIN;
            //Generating Combinations
            std::string bitmask(K, 1); // K leading 1's
            bitmask.resize(N, 0); // N-K trailing 0's
            do {
                set<int> resultSet;
                vector<int> pointSet;
                for (int i = 0; i < N; ++i) // [0..N-1] integers
                {
                    if (bitmask[i]) {
                        set<int> viewshedSet;
                        vector<int> resultVector;
                        Point refPoint = elevationBucket.allPointsInBucket[i];
                        long ithIndex = refPoint.row * nCols + refPoint.col;
                        switch (INSIDE_AND_OUTSIDE){
                            case 1:
                                for(long j = 0; j < visibilityMatrixNodes; j++){
                                    double distance = getDistance(refPoint, getPoint(j, nCols));
                                    if(elevationBucket.allPointsInBucket.size() == 1){
                                        break;
                                    }
                                    if(ithIndex == j){
                                        continue;
                                    }
                                    if(visibilityRange != -1 && distance > visibilityRange){
                                        continue;
                                    }
                                    if(getBit(visibilityMatrix, ithIndex * visibilityMatrixNodes + j)){
                                        viewshedSet.insert(j);
                                    }
                                }
                                break;
                            case 0:
                                for(long j = 0; j < elevationBucket.allPointsInBucket.size(); j++){
                                    Point checkForVisibility = elevationBucket.allPointsInBucket[j];
                                    if(refPoint == checkForVisibility){
                                        continue;
                                    }
                                    int jthIndex = checkForVisibility.row * nCols + checkForVisibility.col;
                                    if(getBit(visibilityMatrix, ithIndex * visibilityMatrixNodes + jthIndex)){
                                        viewshedSet.insert(jthIndex);
                                    }
                                }
                                break;
                        }
                        set_union (viewshedSet.begin(), viewshedSet.end(), resultSet.begin(), resultSet.end(), back_inserter(resultVector));
                        //copy items from vector to result set to again calculate the union
                        resultSet.clear();
                        for(int k = 0; k < resultVector.size(); k++){
                            resultSet.insert(resultVector[k]);
                        }
                        pointSet.push_back(ithIndex);
                    }
                }
                long currentSizeOfSet = resultSet.size();
                if(currentSizeOfSet > maximumUnion){
                    maximumUnion = resultSet.size();
                    nonRemovedPoints.clear();
                    copy(pointSet.begin(), pointSet.end(), back_inserter(nonRemovedPoints));
                }
            } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
            for(int i = 0; i < nonRemovedPoints.size(); i++){
                mutedArray[nonRemovedPoints[i]] = false;
            }
//            cout << maximumUnion << endl;
            tempSize += nonRemovedPoints.size();
        }
        nonRemovedPoints.clear();
    }
     cout << "Total Buckets: " << allElevationBucketsMap.size() << endl;
     cout << "Start Removing Globally" << endl;
    //global Removal from all buckets of same size
    for (map<int,vector<Point>>::iterator it = allElevationBucketsMap.begin(); it != allElevationBucketsMap.end(); ++it){
        cout << "Bucket Index: " << it->first << ", Size of points: " << it->second.size() << endl;
        vector<int> nonRemovedPoints;
        if(it->second.size() > COMBINATION_THRESHOLD){
            int remainingPoints = ceil( it->second.size() * (1 - (removalPercent / 100.0)) );
            int iterationToCheckMaxViewshed = 2;//ceil(it->second.size() / remainingPoints);
            long maximumUnion = INT_MIN;
            for(int l = 0; l < iterationToCheckMaxViewshed; l++){
                random_shuffle ( it->second.begin(), it->second.end());
                set<int> resultSet;
                vector<int> pointSet;
                for(int i = 0; i < remainingPoints; i++){
                    set<int> viewshedSet;
                    vector<int> resultVector;
                    Point refPoint = it->second[i];
                    long ithIndex = refPoint.row * nCols + refPoint.col;
                    switch (INSIDE_AND_OUTSIDE){
                        case 1:
                            for(long j = 0; j < visibilityMatrixNodes; j++){
                                double distance = getDistance(refPoint, getPoint(j, nCols));
                                if(ithIndex == j){
                                    continue;
                                }
                                if(visibilityRange != -1 && distance > visibilityRange){
                                    continue;
                                }
                                if(getBit(visibilityMatrix, ithIndex * visibilityMatrixNodes + j)){
                                    viewshedSet.insert(j);
                                }
                            }
                            break;
                        case 0:
                            for(long j = 0; j < it->second.size(); j++){
                                Point checkForVisibility = it->second[j];
                                if(refPoint == checkForVisibility){
                                    continue;
                                }
                                int jthIndex = checkForVisibility.row * nCols + checkForVisibility.col;
                                if(getBit(visibilityMatrix, ithIndex * visibilityMatrixNodes + jthIndex)){
                                    viewshedSet.insert(jthIndex);
                                }
                            }
                            break;
                    }
                    set_union (viewshedSet.begin(), viewshedSet.end(), resultSet.begin(), resultSet.end(), back_inserter(resultVector));
                    //copy items from vector to result set to again calculate the union
                    resultSet.clear();
                    for(int k = 0; k < resultVector.size(); k++){
                        resultSet.insert(resultVector[k]);
                    }
                    pointSet.push_back(ithIndex);
                }
                long currentSizeOfSet = resultSet.size();
                if(currentSizeOfSet > maximumUnion){
                    maximumUnion = resultSet.size();
                    nonRemovedPoints.clear();
                    copy(pointSet.begin(), pointSet.end(), back_inserter(nonRemovedPoints));
                }
            }
            for(int i = 0; i < nonRemovedPoints.size(); i++){
                mutedArray[nonRemovedPoints[i]] = false;
            }
//            cout << maximumUnion << endl;
            for(int m = 0; m < it->second.size(); m++){
                bool pointExist = false;
                for(int n = 0; n < nonRemovedPoints.size(); n++){
                    if((it->second[m].row * nCols + it->second[m].col) == nonRemovedPoints[n]){
                        pointExist = true;
                        break;
                    }
                }
                if(!pointExist){
                    mutedArray[it->second[m].row * nCols + it->second[m].col] = true;
                }
            }
        }else{
            int N = it->second.size();
            int K = ceil(N * (1 - (removalPercent / 100.0)));
            long maximumUnion = INT_MIN;
            //Generating Combinations
            std::string bitmask(K, 1); // K leading 1's
            bitmask.resize(N, 0); // N-K trailing 0's
            do {
                set<int> resultSet;
                vector<int> pointSet;
                for (int i = 0; i < N; ++i) // [0..N-1] integers
                {
                    if (bitmask[i]) {
                        set<int> viewshedSet;
                        vector<int> resultVector;
                        Point refPoint = it->second[i];
                        long ithIndex = refPoint.row * nCols + refPoint.col;
                        switch (INSIDE_AND_OUTSIDE){
                            case 1:
                                for(int j = 0; j < visibilityMatrixNodes; j++){
                                    if(it->second.size() == 1){
                                        break;
                                    }
                                    double distance = getDistance(refPoint, getPoint(j, nCols));
                                    if(ithIndex == j){
                                        continue;
                                    }
                                    if(visibilityRange != -1 && distance > visibilityRange){
                                        continue;
                                    }
                                    if(getBit(visibilityMatrix, ithIndex * visibilityMatrixNodes + j)){
                                        viewshedSet.insert(j);
                                    }
                                }
                                break;
                            case 0:
                                for(int j = 0; j < it->second.size(); j++){
                                    Point checkForVisibility = it->second[j];
                                    if(refPoint == checkForVisibility){
                                        continue;
                                    }
                                    int jthIndex = checkForVisibility.row * nCols + checkForVisibility.col;
                                    if(getBit(visibilityMatrix, ithIndex * visibilityMatrixNodes + jthIndex)){
                                        viewshedSet.insert(jthIndex);
                                    }
                                }
                                break;
                        }
                        set_union (viewshedSet.begin(), viewshedSet.end(), resultSet.begin(), resultSet.end(), back_inserter(resultVector));
                        //copy items from vector to result set to again calculate the union
                        resultSet.clear();
                        for(int k = 0; k < resultVector.size(); k++){
                            resultSet.insert(resultVector[k]);
                        }
                        pointSet.push_back(ithIndex);
                    }
                }
                long currentSizeOfSet = resultSet.size();
                if(currentSizeOfSet > maximumUnion){
                    maximumUnion = resultSet.size();
                    nonRemovedPoints.clear();
                    copy(pointSet.begin(), pointSet.end(), back_inserter(nonRemovedPoints));
                }
            } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
            for(int i = 0; i < nonRemovedPoints.size(); i++){
                mutedArray[nonRemovedPoints[i]] = false;
            }

            for(int m = 0; m < it->second.size(); m++){
                bool pointExist = false;
                for(int n = 0; n < nonRemovedPoints.size(); n++){
                    if((it->second[m].row * nCols + it->second[m].col) == nonRemovedPoints[n]){
                        pointExist = true;
                        break;
                    }
                }
                if(!pointExist){
                    mutedArray[it->second[m].row * nCols + it->second[m].col] = true;
                }
            }
        }
        nonRemovedPoints.clear(); // clearing for the next combination
    }

    cout << "Starting Writing: " << tempSize + tempSize2 << endl;
    //writing removed points to file
    for(int i = 0; i < visibilityMatrixNodes; i++){
        if(noDataValuesArray[i]){
            mutedArray[i] = false;
        }
    }
    for(int i = 0; i < visibilityMatrixNodes; i++){
        if(mutedArray[i]){
            mutedPointsFile << i << endl;
        }
    }

    mutedPointsFile.close();

    char appendedNewName[15] = "Remaining.txt\0";
    char nonRemovedFilePath[300] = {'\0'};
    strcpy(nonRemovedFilePath, argv[5]);
    nonRemovedFilePath[strlen(nonRemovedFilePath)-4] = '\0';
    strcat(nonRemovedFilePath, appendedNewName);
    ofstream nonRemovedFileHandel(nonRemovedFilePath);
    for(int i = 0; i < visibilityMatrixNodes; i++){
        if(!mutedArray[i]){
            nonRemovedFileHandel << i << endl;
        }
    }
    nonRemovedFileHandel.close();
    cout << "calculation done" << endl;
//    int connected_nodes = 0;
//    int totalNonMutedNodes = 0; int nonMutedNodesConnectivity = 0;

//    for(int i = 0; i < nRows; i++){
//        for(int j = 0; j < nCols; j++){
//            if(!mutedArray[i * nCols + j] && !noDataValuesArray[i * nCols + j]){
//                long ithIndex = i * nCols + j;
//                int nonMutedNodeCon = checkVisibility(mutedArray,ithIndex, nRows, nCols, visibilityMatrix, visibilityMatrixNodes, noDataValuesArray);
//                if(nonMutedNodeCon > 0){
//                    nonMutedNodesConnectivity++;
//                }
//                totalNonMutedNodes++;
//            }
//        }
//    }
//    cout << "NC: " << totalNonMutedNodes << ", TNC: " << nonMutedNodesConnectivity << endl;
//    for(int j = 0; j < visibilityMatrixNodes; j++){
//        for(int i = 0; i < visibilityMatrixNodes; i++){
//
//            if(mutedArray[i]){
//
//                continue;
//            }
//
//            if(i == j){
//                continue;
//            }
//
//            if(getBit(visibilityMatrix, i * visibilityMatrixNodes + j)){
//                connected_nodes++;
//                break;
//            }
//        }
//    }

//    double percentC = ((double)connected_nodes / (double(nRows * nCols) - (double)totalNoDataValues)) * 100.0;
//    double percentNMC = ((double)nonMutedNodesConnectivity / (double)totalNonMutedNodes) * 100.0;
//    int totalMutedPoints = (visibilityMatrixNodes - totalNoDataValues - totalNonMutedNodes);
//    double percentOfTotalRemoval = ( (totalMutedPoints * 100.0) /  (visibilityMatrixNodes - totalNoDataValues ) );
//
//    cout << "C: " << percentC << " %" << endl;
//    cout << percentC<<","<<percentNMC<<","<<percentOfTotalRemoval<<endl;

    delete[] elevationBuckets;
    //cout << nCols << endl;
    for(int i = 0; i < nRows; i++){
        free(elevationMap[i]);
        free(trackingMap[i]);
    }
    free(elevationMap);
    free(trackingMap);
    free(visibilityMatrix);
    return 0;
}