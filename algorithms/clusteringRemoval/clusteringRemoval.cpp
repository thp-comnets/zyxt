#include <iostream>
#include <vector>
#include <climits>
#include <fstream>
#include <queue>
#include <math.h>
#include <map>
#include <algorithm>
#include <set>
#include <sstream>
#include <unordered_set>
#include <sys/time.h>
#include "Graph.cpp"

using namespace std;

#define BUCKET_SIZE 100
#define NO_DATA 65535
double xDist = 0, yDist = 0;


struct GPSPoint{
    double lat;
    double lng;
    GPSPoint(){
        this->lat = 0.0;
        this->lng = 0.0;
    }
    void print(){
        cout << this->lng << "," << this->lat << endl;
    }
};



struct Point{
    int row;
    int col;
    int elevation;
    long visibilityCount;
    long parentVisibilityCounter;
    long withinVisibilityCounter;
    long sinkVisibilityCounter;
    int visibilityPercent;
    int clusterIndex;
    GPSPoint gpsPoint;
    Point(){
        this->row = 0;
        this->col = 0;
        this->clusterIndex = 0;
        this->visibilityCount = 0;
        this->elevation = 0;
        this->parentVisibilityCounter = 0;
        this->withinVisibilityCounter = 0;
        this->visibilityPercent = 0;
        this->sinkVisibilityCounter = 0;
        this->gpsPoint.lat = 0.0;
        this->gpsPoint.lng = 0.0;
    }

    bool isEmpty(){
        if(this->row == 0 && this->col == 0 && this->elevation == 0){
            return true;
        }
        return false;
    }

    void calculateVisibilityPercent(long totalNodes){
        this->visibilityPercent = int(ceil(( (double)visibilityCount / (double)totalNodes ) * 100.0));
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



double rad2Deg(double rad){
    return rad * 180 / M_PI;
}

double deg2rad(double deg) {
    return deg * (M_PI/180);
}



double calculateDistance(GPSPoint p1, GPSPoint p2){
    double R = 6371; // Radius of the earth in km
    double dLat = deg2rad(p2.lat-p1.lat);  // deg2rad below
    double dLon = deg2rad(p2.lng-p1.lng);
    double a = sin(dLat/2) * sin(dLat/2) + cos(deg2rad(p1.lat)) * cos(deg2rad(p2.lat)) * sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return (R * c); // Distance in km
}



struct less_than_key
{
    inline bool operator() (const Point& struct1, const Point& struct2)
    {
        return (struct1.parentVisibilityCounter < struct2.parentVisibilityCounter);
    }
};

struct within_visibility_counter
{
    inline bool operator() (const Point& struct1, const Point& struct2)
    {
        return (struct1.withinVisibilityCounter < struct2.withinVisibilityCounter);
    }
};

struct sink_visibility_counter
{
    inline bool operator() (const Point& struct1, const Point& struct2)
    {
        return (struct1.sinkVisibilityCounter < struct2.sinkVisibilityCounter);
    }
};


struct Cluster{
    GPSPoint centeroid;
    Point selectedPoint;
    vector<Point> clusterPoints;
    double distanceFromOther;
    Cluster(){
        this->distanceFromOther = 0.0;
    }
    void findCenteroid(){
        double sumX = 0;
        double sumY = 0;
        double sumZ = 0;
        for(int i = 0; i < this->clusterPoints.size(); i++){
            double latRad = deg2rad(this->clusterPoints[i].gpsPoint.lat);
            double lngRad = deg2rad(this->clusterPoints[i].gpsPoint.lng);
            // sum of cartesian coordinates
            sumX += cos(latRad) * cos(lngRad);
            sumY += cos(latRad) * sin(lngRad);
            sumZ += sin(latRad);
        }
        double avgX = sumX / this->clusterPoints.size();
        double avgY = sumY / this->clusterPoints.size();
        double avgZ = sumZ / this->clusterPoints.size();
        double lng_n = atan2(avgY, avgX);
        double hyp = sqrt(avgX * avgX + avgY * avgY);
        double lat_n = atan2(avgZ, hyp);

        this->centeroid.lat = rad2Deg(lat_n);
        this->centeroid.lng = rad2Deg(lng_n);
        cout.precision(15);
//        cout << "New Centroid is: " << this->centeroid.lng << "," << this->centeroid.lat << endl;
    }

    bool operator== (const Cluster &c1) const {
        return (this->distanceFromOther == c1.distanceFromOther);
    }

    bool operator< (const Cluster &c2) const
    {
        return this->distanceFromOther < c2.distanceFromOther;
    }

    bool operator<= (const Cluster &c2) const
    {
        return this->distanceFromOther <= c2.distanceFromOther;
    }

};

struct BoundingBox{
    GPSPoint Tleft;
    GPSPoint Tright;
    GPSPoint BLeft;
    GPSPoint BRight;

    BoundingBox(vector<Point> allPoints){
        GPSPoint minx, miny, maxx, maxy;
        minx.lat = INT_MAX; miny.lng = INT_MAX;
        maxx.lat = INT_MIN, maxy.lng = INT_MIN;
        for(int i = 0; i < allPoints.size(); i++){
            if(allPoints[i].gpsPoint.lat < minx.lat){
                minx = allPoints[i].gpsPoint;
            }
            if(allPoints[i].gpsPoint.lng < miny.lng){
                miny = allPoints[i].gpsPoint;
            }
            if(allPoints[i].gpsPoint.lat > maxx.lat){
                maxx = allPoints[i].gpsPoint;
            }
            if(allPoints[i].gpsPoint.lng > maxy.lng){
                maxy = allPoints[i].gpsPoint;
            }
        }
        this->Tleft.lat = maxx.lat; this->Tleft.lng = miny.lng;
        this->Tright.lat = maxx.lat; this->Tright.lng = maxy.lng;
        this->BLeft.lat = minx.lat; this->BLeft.lng = miny.lng;
        this->BRight.lat = minx.lat; this->BRight.lng = maxy.lng;
    }

    bool isWithinBound(Point point){
        return point.gpsPoint.lng >= this->Tleft.lng && point.gpsPoint.lng <= this->Tright.lng &&
        point.gpsPoint.lat <= this->Tleft.lat && point.gpsPoint.lat >= this->BLeft.lat;
    }
};


struct BucketRange{
    int bucketIndex;
    int start;
    int end;
    void calculateRange(){
        this->start = (this->bucketIndex * BUCKET_SIZE);
        this->end = (this->start + (BUCKET_SIZE-1));
    }
};

struct ElevationBucket{
    BucketRange bucketRange;
    vector<Point> allPointsInBucket;
    vector<Point> remainingPoints;
    map<int, ElevationBucket*> parentBuckets;
};

void my_swap(float &x, float &y){
    float temp = 0.0;
    temp = x;
    x = y;
    y = temp;
}

vector<Point> drawline(Point start, Point end) {

    vector<Point> allPointsInLine;
    float x1 = start.row, y1 = start.col, x2 = end.row, y2 = end.col;
    const bool steep = (fabs(y2 - y1) > fabs(x2 - x1));
    if(steep)
    {
        my_swap(x1, y1);
        my_swap(x2, y2);
    }

    if(x1 > x2)
    {
        my_swap(x1, x2);
        my_swap(y1, y2);
    }

    const float dx = x2 - x1;
    const float dy = fabs(y2 - y1);

    float error = dx / 2.0f;
    const int ystep = (y1 < y2) ? 1 : -1;
    int y = (int)y1;

    const int maxX = (int)x2;
    int steps = 0;
    for(int x=(int)x1; x<maxX; x++)
    {
        Point newP;
        if(steep)
        {
            newP.row = (int)y;
            newP.col = (int)x;
        }
        else
        {
            newP.row = (int)x;
            newP.col = (int)y;
        }
        error -= dy;
        if(error < 0)
        {
            y += ystep;
            error += dx;
        }
        steps++;
        allPointsInLine.push_back(newP);
    }
    return allPointsInLine;
}

bool isValidEdge(Point from, Point to, int **elevationMap, int desiredElevationIndex){

    vector<Point> allPointsInLine = drawline(from, to);
    for(int i = 0; i < allPointsInLine.size(); i++){
        int pointElevationIndex = elevationMap[ allPointsInLine[i].row ][ allPointsInLine[i].col ] / BUCKET_SIZE;
        if(desiredElevationIndex != pointElevationIndex){
            return false;
        }
    }
    return true;
}

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

int getIndexOfBucktInBucketMap(vector<ElevationBucket> &elevationBuckets, Point point){

    for(int i = 0; i < elevationBuckets.size(); i++){
        for(int j = 0; j < elevationBuckets[i].allPointsInBucket.size(); j++){
            if(elevationBuckets[i].allPointsInBucket[j] == point){
                return i;
            }
        }
    }

}

vector<Point> getNeighbours(int ** elevationMap, Point startPoint, int nRow, int nCol, bool** trackingMap, int oneLevelUpIndex, map<int, vector<ElevationBucket>, greater<int>> &allElevationBucketMap, ElevationBucket &currentBucket){
    vector<Point> neighbours;
    Point top, left, right, bottom /*, topLeft, bottomLeft, topRight, bottomRight */;
    if(startPoint.row - 1 >= 0){ // else no top exist
        top.row = startPoint.row - 1;
        top.col = startPoint.col;
        top.elevation = elevationMap[top.row][top.col];
        if((top.elevation / BUCKET_SIZE) == oneLevelUpIndex ){
            int indexOfParentBucket = getIndexOfBucktInBucketMap(allElevationBucketMap[oneLevelUpIndex], top);
            currentBucket.parentBuckets[indexOfParentBucket] = &allElevationBucketMap[oneLevelUpIndex][indexOfParentBucket];
        }
        if(!trackingMap[top.row][top.col] && top.elevation != NO_DATA){
            neighbours.push_back(top);
        }
    }
    if(startPoint.col - 1 >= 0){ // else no left exist
        left.row = startPoint.row;
        left.col = startPoint.col - 1;
        left.elevation = elevationMap[left.row][left.col];
        if((left.elevation / BUCKET_SIZE) == oneLevelUpIndex){
            int indexOfParentBucket = getIndexOfBucktInBucketMap(allElevationBucketMap[oneLevelUpIndex], left);
            currentBucket.parentBuckets[indexOfParentBucket] = &allElevationBucketMap[oneLevelUpIndex][indexOfParentBucket];
        }
        if(!trackingMap[left.row][left.col] && left.elevation != NO_DATA){
            neighbours.push_back(left);
        }

    }
    if(startPoint.row + 1 < nRow){ // else no bottom exist
        bottom.row = startPoint.row + 1;
        bottom.col = startPoint.col;
        bottom.elevation = elevationMap[bottom.row][bottom.col];
        if((bottom.elevation / BUCKET_SIZE) == oneLevelUpIndex){
            int indexOfParentBucket = getIndexOfBucktInBucketMap(allElevationBucketMap[oneLevelUpIndex], bottom);
            currentBucket.parentBuckets[indexOfParentBucket] = &allElevationBucketMap[oneLevelUpIndex][indexOfParentBucket];
        }
        if(!trackingMap[bottom.row][bottom.col] && bottom.elevation != NO_DATA){
            neighbours.push_back(bottom);
        }
    }
    if(startPoint.col + 1 < nCol){ // else no right exist
        right.row = startPoint.row;
        right.col = startPoint.col + 1;
        right.elevation = elevationMap[right.row][right.col];
        if((right.elevation / BUCKET_SIZE) == oneLevelUpIndex){
            int indexOfParentBucket = getIndexOfBucktInBucketMap(allElevationBucketMap[oneLevelUpIndex], right);
            currentBucket.parentBuckets[indexOfParentBucket] = &allElevationBucketMap[oneLevelUpIndex][indexOfParentBucket];
        }
        if(!trackingMap[right.row][right.col] && right.elevation != NO_DATA){
            neighbours.push_back(right);
        }
    }

    return neighbours;
}


Point getAvailablePoint(bool **trackingMap, int nRows, int nCols, BucketRange bucketRange, int **elevationMap){
    Point p;
    p.row = -1;
    for(int i = 0; i < nRows; i++){
        for(int j = 0; j < nCols; j++){
            if(elevationMap[i][j] != NO_DATA && !trackingMap[i][j] && elevationMap[i][j] >= bucketRange.start && elevationMap[i][j] <= bucketRange.end){
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

void readIndexMapping(GPSPoint *allCoordinates, char* mainPath){
    char filePathBuff[100] = {'\0'};
    sprintf(filePathBuff, "%s/index_mapping.txt", mainPath);
    ifstream indexMappingFileHandel(filePathBuff);
    int ind = 0;
    string line;
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
}

void writeRemainingPoints(char *mainPath, GPSPoint *allCoordinates, long rows, long cols, int totalNoDataValues, map<int, vector<ElevationBucket>, greater<int>> allElevationBucketMap){
    char filePathBuff[100] = {'\0'};
    cout << "Start Writing Remaining Points" << endl;
    filePathBuff[0] = '\0';
    sprintf(filePathBuff, "%s/clusteringRemovalRemaining.txt", mainPath);
    cout << "Writing To: " << filePathBuff << endl;
    ofstream newElevationRemainingFile(filePathBuff);
    newElevationRemainingFile.precision(15);
    long totalRemaining = 0;
    for (auto it = allElevationBucketMap.begin(); it != allElevationBucketMap.end(); ++it) {
        cout << "Writing Remaining Points From Buckets Of Size: " << it->first << endl;
        for (int j = 0; j < it->second.size(); j++) {
            for(int k = 0; k < it->second[j].remainingPoints.size(); k++){
                totalRemaining++;
                long refPoint = it->second[j].remainingPoints[k].row * cols + it->second[j].remainingPoints[k].col;
                newElevationRemainingFile << refPoint << endl;
                //newElevationRemainingFile << allCoordinates[refPoint].lng << "," << allCoordinates[refPoint].lat << "," << refPoint << endl;
//                cout << it->second[j].remainingPoints[k].row * nCols + it->second[j].remainingPoints[k].col << endl;
            }
        }
    }
    newElevationRemainingFile.close();
    cout << "Total Remaining: " << totalRemaining << endl;
    cout << "Total Removed: " << (rows * cols) - totalNoDataValues - totalRemaining << endl;
}

void writePointsInTheFile(char *mainPath, GPSPoint *allCoordinates, vector<Point> allPoints, int eIndex, int bucketIndex, long row, long cols){
    char filePathBuff[100] = {'\0'};
    sprintf(filePathBuff, "%s/sample/testElevation_%d_%d.txt", mainPath, eIndex, bucketIndex);
    ofstream testFile(filePathBuff);
    testFile.precision(15);
    for(auto it = allPoints.begin(); it != allPoints.end(); ++it){
        long refPoint = it->row * cols + it->col;
        testFile << allCoordinates[ refPoint ].lng << "," << allCoordinates[ refPoint ].lat << "," << refPoint << endl;
    }
    testFile.close();
}



std::unordered_set<int> pickSet(int N, int k, std::mt19937& gen)
{
    std::uniform_int_distribution<> dis(0, N-1);
    std::unordered_set<int> elems;

    while (elems.size() < k) {
        elems.insert(dis(gen));
    }

    return elems;
}


Point getNearestClusterIndex(vector<Cluster> allCluster, int refIndex){
    Point selectedPoint;
    allCluster[refIndex].distanceFromOther = 0.0;
    for(int i = 0; i < allCluster.size(); i++){
        if(i == refIndex){
            continue;
        }
        allCluster[i].distanceFromOther = calculateDistance(allCluster[refIndex].centeroid, allCluster[i].centeroid);
    }
    sort(allCluster.begin(), allCluster.end());
    for(int i = 1; i <  allCluster.size(); i++){
        if(!allCluster[i].selectedPoint.isEmpty()){
            selectedPoint = allCluster[i].selectedPoint;
            break;
        }
    }
    return selectedPoint;
}

int getClusterIndexWRTBucketFirstPoint(vector<Point> bucketPoints, vector<Cluster> allClusters){
    for (auto &bucketPoint : bucketPoints) {
        for(int bj = 0; bj < allClusters.size(); bj++) {
            for (int bk = 0; bk < allClusters[bj].clusterPoints.size(); bk++) {
                if(bucketPoint == allClusters[bj].clusterPoints[bk]){
                    return bj;
                }
            }
        }
    }
    return -1;
}

bool isPointInCluster(vector<Point> clusterPoints, Point pointToCheck){
    int i, j;
    bool c = false;
    for (i = 0, j = clusterPoints.size() - 1; i < clusterPoints.size(); j = i++)
    {
        if ((((clusterPoints[i].gpsPoint.lat <= pointToCheck.gpsPoint.lat) && (pointToCheck.gpsPoint.lat < clusterPoints[j].gpsPoint.lat))
             || ((clusterPoints[j].gpsPoint.lat <= pointToCheck.gpsPoint.lat) && (pointToCheck.gpsPoint.lat < clusterPoints[i].gpsPoint.lat)))
            && (pointToCheck.gpsPoint.lng < (clusterPoints[j].gpsPoint.lng - clusterPoints[i].gpsPoint.lng) * (pointToCheck.gpsPoint.lat - clusterPoints[i].gpsPoint.lat)
                           / (clusterPoints[j].gpsPoint.lat - clusterPoints[i].gpsPoint.lat) + clusterPoints[i].gpsPoint.lng)) {

            c = !c;
        }
    }
    return c;
}

// cn_PnPoly(): crossing number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  0 = outside, 1 = inside
// This code is patterned after [Franklin, 2000]
int cn_PnPoly( Point P, vector<Point> V)
{
    int    cn = 0;    // the  crossing number counter
    int i, j;
    bool c = false;
    // loop through all edges of the polygon
    for (i=0, j = V.size()-1; i < V.size(); j = i++) {    // edge from V[i]  to V[i+1]
        if (((V[i].gpsPoint.lat <= P.gpsPoint.lat) && (V[j+1].gpsPoint.lat > P.gpsPoint.lat))     // an upward crossing
            || ((V[j].gpsPoint.lat > P.gpsPoint.lat) && (V[i+1].gpsPoint.lat <=  P.gpsPoint.lat))) { // a downward crossing
            // compute  the actual edge-ray intersect x-coordinate
            float vt = (float)(P.gpsPoint.lat  - V[j].gpsPoint.lat) / (V[i+1].gpsPoint.lat - V[i].gpsPoint.lat);
            if (P.gpsPoint.lng <  V[j].gpsPoint.lng + vt * (V[i+1].gpsPoint.lng - V[i].gpsPoint.lng)) // P.x < intersect
                ++cn;   // a valid crossing of y=P.y right of P.x
        }
    }
    return (cn&1);    // 0 if even (out), and 1 if  odd (in)
}

int main(int argc, char* argv[]){

    ifstream fin;
    char fileNameBuff[200] = {'\0'};
    unsigned char *visibilityMatrix = NULL;
    cout << "Start Reading Elevation Data" << endl;
    sprintf(fileNameBuff, "%s/out.asc", argv[2]);
    fin.open(fileNameBuff);
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
    long nCols = atoi(breakStringByspace(line_ncols).c_str());
    long nRows = atoi(breakStringByspace(line_nrows).c_str());
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
        for(int j = 0; j <  nCols; j++){
            trackingMap[i][j] = false;
        }
    }
    GPSPoint* allCoordinates = new GPSPoint[nRows * nCols];
    readIndexMapping(allCoordinates, argv[2]);
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
    fileNameBuff[0] = '\0';
    sprintf(fileNameBuff, "%s/out_%li.bin", argv[2], visibilityMatrixNodes);
    readBinaryFile(visibilityMatrix, visibilityMatrixNodes, fileNameBuff);
    mt19937 randomEng;
    randomEng.seed(98456);
    int totalNoDataValues = 0;
    fileNameBuff[0] = '\0';
    sprintf(fileNameBuff, "%s/noDataValues.txt", argv[2]);
    ofstream noDataValuesHandler(fileNameBuff);
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
    cout << "Total No Data Points: " << totalNoDataValues << endl;
    noDataValuesHandler.close();

//    //loading sinks
//    random_device rd;
//    mt19937 gen(rd());
//    uniform_int_distribution<long> dist(0, visibilityMatrixNodes-1);
//    vector<long> sinks;
//    cout << "Sinks: ";
//    while(sinks.size() <= 10){
//        long randSink = dist(gen);
//        int row = randSink / nCols;
//        int col = randSink % nCols;
//        if(elevationMap[row][col] != noData){
//            sinks.push_back(randSink);
//            cout << randSink << ", ";
//        }
//
//    }
//    cout << endl;
    queue<Point> neighboursQueue;
    cout << "Start finding areas..." << endl << "*************************" << endl;
    double removalPercent = stod(argv[1]);

    map<int, vector<ElevationBucket>, greater<int>> allElevationBucketMap;
    int maximumIndex = maximumElevation / BUCKET_SIZE;
    while (maximumIndex > -1){
        BucketRange bucketRange;
        bucketRange.bucketIndex = maximumIndex;
        bucketRange.calculateRange();
        while(true){
            ElevationBucket elevationBucket;
            elevationBucket.bucketRange = bucketRange;
            Point availablePoint = getAvailablePoint(trackingMap, nRows, nCols, bucketRange, elevationMap);
            if(availablePoint.row == -1){
                break; // we cover every point
            }
            availablePoint.elevation = elevationMap[availablePoint.row][availablePoint.col];

            availablePoint.gpsPoint = allCoordinates[availablePoint.row * nCols + availablePoint.row];

            trackingMap[availablePoint.row][availablePoint.col] = true;
            elevationBucket.allPointsInBucket.push_back(availablePoint);
            neighboursQueue.push(availablePoint);
            int desiredElevationBucketIndex = availablePoint.elevation / BUCKET_SIZE;
            while (!neighboursQueue.empty()){
                Point startingPoint = neighboursQueue.front();
                neighboursQueue.pop();
                vector<Point> neighboursOfStartingPoints = getNeighbours(elevationMap, startingPoint, nRows, nCols,trackingMap, bucketRange.bucketIndex+1, allElevationBucketMap, elevationBucket);
                for(int i = 0; i < neighboursOfStartingPoints.size(); i++){
                    int bucketIndexOfNeighbour = neighboursOfStartingPoints[i].elevation / BUCKET_SIZE;
                    if(bucketIndexOfNeighbour == desiredElevationBucketIndex){
                        trackingMap[neighboursOfStartingPoints[i].row][neighboursOfStartingPoints[i].col] = true;
                        neighboursOfStartingPoints[i].gpsPoint = allCoordinates[neighboursOfStartingPoints[i].row * nCols + neighboursOfStartingPoints[i].col];
                        elevationBucket.allPointsInBucket.push_back(neighboursOfStartingPoints[i]);
                        neighboursQueue.push(neighboursOfStartingPoints[i]);
                    }
                }
            }
            vector<ElevationBucket> allBucketsOfCurrentBucketIndex = allElevationBucketMap[bucketRange.bucketIndex];
            allBucketsOfCurrentBucketIndex.push_back(elevationBucket);
            allElevationBucketMap[bucketRange.bucketIndex] = allBucketsOfCurrentBucketIndex;
            if(maximumIndex == 1){
                cout << "P: " << elevationBucket.parentBuckets.size() << endl;
                cout << "Area Found" << endl;
                cout << "Size Of Area: " << elevationBucket.allPointsInBucket.size() << endl;
                cout << "Own Elevation: " << availablePoint.elevation / BUCKET_SIZE << endl;
                cout << endl;
            }

        }
        maximumIndex--;
    }

    cout << "Start Removing ..." << endl;
    /**/


    /**/
    struct timeval start, end;
    int rangeIndex=1,bucketIndex=0;
    for (auto it = allElevationBucketMap.begin(); it != allElevationBucketMap.end(); ++it){
        cout << "Removing From Buckets Of Elevation Range: " << it->first << endl;
//        if(it->first != rangeIndex){
//            continue;
//        }
        for(int j = 0; j < it->second.size(); j++){
            cout << "Bucket: " << j << ", Size: " << it->second[j].allPointsInBucket.size() << endl;
//            if(j != bucketIndex){
//                continue;
//            }
            int remainingPoints = (int)ceil( it->second[j].allPointsInBucket.size() * (1 - (removalPercent / 100.0)) );
            vector<Point> parentRemainingPoints;
            if(it->first == (maximumElevation / BUCKET_SIZE)){
                for(int b = 0; b < it->second[j].allPointsInBucket.size(); b++){
                    Point refPoint = it->second[j].allPointsInBucket[b];
                    long ithIndex = refPoint.row * nCols + refPoint.col;
                    for(long k = 0; k < visibilityMatrixNodes; k++) {
                        if (ithIndex == k) {
                            continue;
                        }
                        if(noDataValuesArray[ithIndex]){
                            continue;
                        }
                        if (getBit(visibilityMatrix, (unsigned long long int)ithIndex * visibilityMatrixNodes + k )) {
                            refPoint.visibilityCount++;
                        }
                    }
                    for(long k = 0; k < it->second[j].allPointsInBucket.size(); k++) {
                        if (ithIndex == k) {
                            continue;
                        }
                        if(noDataValuesArray[ithIndex]){
                            continue;
                        }
                        long toIndex = it->second[j].allPointsInBucket[k].row * nCols + it->second[j].allPointsInBucket[k].col;
                        if (getBit(visibilityMatrix, (unsigned long long int)ithIndex * visibilityMatrixNodes + toIndex )) {
                            refPoint.withinVisibilityCounter++;
                        }
                    }

                    it->second[j].allPointsInBucket[b].visibilityCount = refPoint.visibilityCount;
                    it->second[j].allPointsInBucket[b].withinVisibilityCounter = refPoint.withinVisibilityCounter;
                    it->second[j].allPointsInBucket[b].calculateVisibilityPercent(visibilityMatrixNodes);
                    it->second[j].allPointsInBucket[b].gpsPoint.lat = allCoordinates[ithIndex].lat;
                    it->second[j].allPointsInBucket[b].gpsPoint.lng = allCoordinates[ithIndex].lng;
                }

                it->second[j].remainingPoints.clear();
            }else{

                for (auto parentIt = it->second[j].parentBuckets.begin(); parentIt != it->second[j].parentBuckets.end(); ++parentIt){
                    copy(parentIt->second->remainingPoints.begin(), parentIt->second->remainingPoints.end(), back_inserter(parentRemainingPoints));
                }
                for(int b = 0; b < it->second[j].allPointsInBucket.size(); b++){
                    Point refPoint = it->second[j].allPointsInBucket[b];
                    long ithIndex = refPoint.row * nCols + refPoint.col;
                    for (auto &parentRemainingPoint : parentRemainingPoints) {
                        if (refPoint == parentRemainingPoint) {
                            continue;
                        }
                        long jthIndex = parentRemainingPoint.row * nCols + parentRemainingPoint.col;
                        if (getBit(visibilityMatrix, (unsigned long long int)ithIndex * visibilityMatrixNodes + jthIndex )) {
                            refPoint.parentVisibilityCounter++;
                        }
                    }
                    it->second[j].allPointsInBucket[b].parentVisibilityCounter = refPoint.parentVisibilityCounter;
                    //calculating from the whole map
                    for(long k = 0; k < visibilityMatrixNodes; k++) {
                        if (ithIndex == k) {
                            continue;
                        }
                        if(noDataValuesArray[ithIndex]){
                            continue;
                        }
                        if (getBit(visibilityMatrix, (unsigned long long int)ithIndex * visibilityMatrixNodes + k )) {
                            refPoint.visibilityCount++;
                        }
                    }
                    for(long k = 0; k < it->second[j].allPointsInBucket.size(); k++) {
                        if (ithIndex == k) {
                            continue;
                        }
                        if(noDataValuesArray[ithIndex]){
                            continue;
                        }
                        long toIndex = it->second[j].allPointsInBucket[k].row * nCols + it->second[j].allPointsInBucket[k].col;
                        if (getBit(visibilityMatrix, (unsigned long long int)ithIndex * visibilityMatrixNodes + toIndex )) {
                            refPoint.withinVisibilityCounter++;
                        }
                    }

                    it->second[j].allPointsInBucket[b].visibilityCount = refPoint.visibilityCount;
                    it->second[j].allPointsInBucket[b].withinVisibilityCounter = refPoint.withinVisibilityCounter;
                    it->second[j].allPointsInBucket[b].calculateVisibilityPercent(visibilityMatrixNodes);
                    it->second[j].allPointsInBucket[b].gpsPoint.lat = allCoordinates[ithIndex].lat;
                    it->second[j].allPointsInBucket[b].gpsPoint.lng = allCoordinates[ithIndex].lng;
                }

                it->second[j].remainingPoints.clear();
            }

            if(it->second[j].allPointsInBucket.size() > 1){

                gettimeofday(&start, NULL);
                bool allPointsConsider = false;
                int randomPointsToTake;
                if(it->second[j].allPointsInBucket.size() > 300){
                    randomPointsToTake = max(300, remainingPoints * 2);
                }else if(it->second[j].allPointsInBucket.size() > 200){
                    randomPointsToTake = 200;
                }else if(it->second[j].allPointsInBucket.size() > 100){
                    randomPointsToTake = 100;
                }else{
                    allPointsConsider = true;
                    randomPointsToTake = it->second[j].allPointsInBucket.size();
                }


                vector<Point> pointToConsider;
                if(allPointsConsider){
                    copy(it->second[j].allPointsInBucket.begin(), it->second[j].allPointsInBucket.end(), back_inserter(pointToConsider));
                }else {
                    bool *randTrackingMap = new bool[it->second[j].allPointsInBucket.size()]();
                    int tempRandIndex = 0;
                    while (pointToConsider.size() < randomPointsToTake) {
                        uniform_int_distribution<> dis(0, it->second[j].allPointsInBucket.size() - 1);
                        int num = dis(randomEng);//sampleRandom[tempRandIndex];
                        if (!randTrackingMap[num]) {
                            randTrackingMap[num] = true;
                            pointToConsider.push_back(it->second[j].allPointsInBucket[num]);
                            tempRandIndex++;
                        }
                    }
                    delete[]randTrackingMap;
                }
                Graph graph(pointToConsider.size());
                for(long ei = 0; ei < pointToConsider.size(); ei++){
                    long indexFrom = pointToConsider[ei].row * nCols + pointToConsider[ei].col;
                    for(long ej = 0; ej < pointToConsider.size(); ej++){
                        if(ei == ej){
                            continue;
                        }
                        long indexTo = pointToConsider[ej].row * nCols + pointToConsider[ej].col;

                        if(getBit(visibilityMatrix, (unsigned long long int) indexFrom * visibilityMatrixNodes + indexTo)){
                            if(isValidEdge(pointToConsider[ei], pointToConsider[ej], elevationMap, it->first)){
                                double distance = calculateDistance(allCoordinates[ indexFrom ], allCoordinates[ indexTo ]);
                                graph.addEdge(ei, ej, distance);
                            }

                        }
                    }
                }
                double ** distanceMatrix = new double*[randomPointsToTake];
                for(int di = 0; di < randomPointsToTake; di++){
                    distanceMatrix[di] = new double[randomPointsToTake];
                }
                for(int fill = 0; fill <  randomPointsToTake; fill++){
                    graph.shortestPathWithMatrix(fill, distanceMatrix);
                }
                gettimeofday(&end, NULL);
                cout << "Total Time: " << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000 << " Seconds" << endl;
                fileNameBuff[0] = '\0';
                sprintf(fileNameBuff,"%s/distanceMatrix.txt", argv[2]);
                ofstream distanceMatrixOut(fileNameBuff);
                for(int di = 0; di < randomPointsToTake; di++){
                    for(int dj = 0; dj < randomPointsToTake; dj++){
                        distanceMatrixOut << distanceMatrix[di][dj] << ",";
                    }
                    distanceMatrixOut << endl;
                }
                distanceMatrixOut.close();
                for(int di = 0; di < randomPointsToTake; di++){
                    delete [] distanceMatrix[di];
                }
                delete distanceMatrix;
                //run clustering
                char command[200] = {'\0'};
                sprintf(command , "python /home/cuda/Dropbox/wisp_planning_tool/toolChain_2.0/spectralClustering.py %s %d", argv[2], remainingPoints);
                cout << "Command to be run: " << command << endl;
                system(command);

                fileNameBuff[0] = '\0';
                sprintf(fileNameBuff, "%s/spectralClusterResults.txt", argv[2]);
                ifstream clsuetrResultFile(fileNameBuff);
                int readIndex = 0, totalClust = 0, numFile;
                vector<int> clusterLabels;
                while(clsuetrResultFile.good() && readIndex < randomPointsToTake){
                    if(readIndex == 0){
                        clsuetrResultFile >> totalClust;
                    }else{
                        clsuetrResultFile >> numFile;
                        clusterLabels.push_back(numFile);
                    }
                    readIndex++;
                }
                cout << "total clusters: " << totalClust << endl;
                clsuetrResultFile.close();
                vector<Cluster> allClusters;
                for(int ci = 0; ci < totalClust; ci++){
                    Cluster cluster_;

                    for(int cj = 0; cj < clusterLabels.size(); cj++){
                        if(clusterLabels[cj] == ci){
                            pointToConsider[cj].clusterIndex = cj;
                            cluster_.clusterPoints.push_back( pointToConsider[cj] );
                        }
                    }
                    cluster_.findCenteroid();
                    allClusters.push_back(cluster_);
                }
                cout << "Centroids calculations done" << endl;


                //sorting of clusters
                int firstClusterIndex = getClusterIndexWRTBucketFirstPoint(it->second[j].allPointsInBucket, allClusters);
                if(firstClusterIndex == -1){
                    cout << "No first cluster found..." << endl;
                    exit(1);
                }
                cout << "First Cluster Index: " << firstClusterIndex << endl;
                allClusters[firstClusterIndex].distanceFromOther = 0.0;
                for(int aci = 0; aci <  allClusters.size(); aci++){
                    if(aci == firstClusterIndex){
                        continue;
                    }
                    double distance = calculateDistance(allClusters[firstClusterIndex].centeroid, allClusters[aci].centeroid);
                    allClusters[aci].distanceFromOther = distance;
                }
                sort(allClusters.begin(), allClusters.end());
                cout << "Sorting of clusters done" << endl;
////            finding all points in each cluster region by doing bounding business.
                for (auto &clust : allClusters){
                    BoundingBox boundingBox(clust.clusterPoints);
                    clust.clusterPoints.clear();
                    for(int i = 0; i < it->second[j].allPointsInBucket.size(); i++){
                        if(boundingBox.isWithinBound(it->second[j].allPointsInBucket[i])){
                            clust.clusterPoints.push_back(it->second[j].allPointsInBucket[i]);
                        }
                    }
                }

                ////printing of all cluster points
//                cout.precision(15);
//                for (auto &clust : allClusters){
//                    cout << "c: " << clust.centeroid.lng << "," << clust.centeroid.lat << endl;
////                  cout << "***********BB**********" << endl;
////                  boundingBox.Tleft.print(); boundingBox.Tright.print(); boundingBox.BLeft.print(); boundingBox.BRight.print();
////                  cout << "***********************" << endl;
//
//                    for(int acj = 0; acj < clust.clusterPoints.size(); acj++){
//                        clust.clusterPoints[acj].gpsPoint.print();
//                    }
//                    cout << "**********"<< endl;
//                }
//                exit(1);
                double totalDistance = 0.0, meanDistance;
                unsigned long clusterSize = allClusters.size() - 1;
                double minDistance = DBL_MAX;
                for(int aci = 1; aci < allClusters.size(); aci++){
                    double distance = calculateDistance(allClusters[aci-1].centeroid, allClusters[aci].centeroid);
                    if(distance < minDistance){
                        minDistance = distance;
                    }
                    totalDistance += distance;
                }
                meanDistance = minDistance * stod(argv[3]);
                cout << "Mean Distance Between Clusters: " << meanDistance << endl;
                //sorting of clusters
                if(parentRemainingPoints.empty()){ //if bucket belongs to max elevation then sort all clusters only for visibility count
                    for (auto &cluster : allClusters) {
                        sort(cluster.clusterPoints.begin(), cluster.clusterPoints.end());
                    }

                }else{ //if bucket is low elevation buckets, then sort all clusters according to visibility of parent and then to whole map.
                    for (auto &cluster : allClusters) {
                        sort(cluster.clusterPoints.begin(), cluster.clusterPoints.end(), less_than_key());
                        //sort the partitions
                        if(cluster.clusterPoints.size() > 1){
                            long bucketIndex = cluster.clusterPoints.size()-1;
                            for(long s = parentRemainingPoints.size(); s >= 0 ; s--){
                                long startIndex = bucketIndex;
                                long endIndex;
                                long a = bucketIndex;
                                for(; a >= 0; a--){
                                    if(cluster.clusterPoints[a].parentVisibilityCounter != s){
                                        break;
                                    }
                                }
                                if(a != bucketIndex){
                                    endIndex = a+1;
                                    bucketIndex = a;
                                    sort(cluster.clusterPoints.begin()+endIndex, cluster.clusterPoints.begin()+(startIndex+1));
                                }
                            }
                        }
                    }
                }
//                Remaining Point Selection
                GPSPoint refPointToSelection;
                while(it->second[j].remainingPoints.size() < remainingPoints){
                    if(it->second[j].remainingPoints.empty()){
                        it->second[j].remainingPoints.push_back( allClusters[0].clusterPoints[ allClusters[0].clusterPoints.size()-1 ] );
                        allClusters[0].selectedPoint = allClusters[0].clusterPoints[ allClusters[0].clusterPoints.size()-1 ];
                        refPointToSelection = allClusters[0].clusterPoints[ allClusters[0].clusterPoints.size()-1 ].gpsPoint;

                    }else{
                        for(int aci = 1; aci < allClusters.size(); aci++){
                            for(long acj = allClusters[aci].clusterPoints.size() - 1; acj >= 0; acj--){
                                double distance = calculateDistance(refPointToSelection, allClusters[aci].clusterPoints[acj].gpsPoint);
                                if (distance >= meanDistance){
                                    it->second[j].remainingPoints.push_back( allClusters[aci].clusterPoints[acj] );
                                    refPointToSelection = allClusters[aci].clusterPoints[acj].gpsPoint;
                                    break;
                                }
                            }
                        }

                    }
                }
                cout << "Point Selection" << endl;
                cout.precision(15);
                for(int ri = 0; ri < it->second[j].remainingPoints.size(); ri++){
                    cout << it->second[j].remainingPoints[ri].gpsPoint.lng << "," << it->second[j].remainingPoints[ri].gpsPoint.lat << endl;
                }
            }else{
                copy(it->second[j].allPointsInBucket.begin(), it->second[j].allPointsInBucket.end(), back_inserter(it->second[j].remainingPoints));
            }
        }
    }

    writeRemainingPoints(argv[2], allCoordinates, nRows, nCols, totalNoDataValues, allElevationBucketMap);


    for(int i = 0; i < nRows; i++){
        free(elevationMap[i]);
        free(trackingMap[i]);
    }
    free(elevationMap);
    free(trackingMap);
    free(visibilityMatrix);
    delete []allCoordinates;
    return 0;
}