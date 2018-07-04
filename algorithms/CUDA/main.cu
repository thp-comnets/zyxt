#include <fstream>
#include <string>
#include <iostream>
#include <stdint.h>
#include <math.h>
#include <utility>
#include <sys/time.h>
#include <limits>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <map>

#include <bitset>
using namespace std;

#define STEPSIZE 1 // step size in pixels, e.g. 2 = every second pixel

// compile with:
// make && ./main s11e121.txt 0 0

struct Point{
    short row;
    short col;
};

struct Pixel{
    int elevation;
    int mountingHeight;
};

struct Terrain{
    short nRows;
    short nCols;
    float cellsize;
    float xllcorner;
    float yllcorner;
    double noDataValue;
    Point *towerLocations;
    Pixel *gridTerrian;
};

clock_t start = 0, endt;
double elapsed;

void Print_Time() {
  endt = clock();
  elapsed = ((double)(endt - start)) / CLOCKS_PER_SEC;
  start = endt;

  cerr << "GPU Time: " << elapsed << endl;
}

#define CUDA_CALL(cuda_function, ...)  { \
    cudaError_t status = cuda_function(__VA_ARGS__); \
    cudaEnsureSuccess(status, #cuda_function, true, __FILE__, __LINE__); \
}

bool cudaEnsureSuccess(cudaError_t status, const char* status_context_description,
        bool die_on_error, const char* filename, unsigned line_number) {
    if (status_context_description == NULL)
        status_context_description = "";
    if (status == cudaSuccess) {
#if REPORT_CUDA_SUCCESS
         cerr <<  "Succeeded: " << status_context_description << std::endl << std::flush;
#endif
        return true;
    }
    const char* errorString = cudaGetErrorString(status);
    cerr << "CUDA Error: ";
    if (status_context_description != NULL) {
        cerr << status_context_description << ": ";
    }
    if (errorString != NULL) {
        cerr << errorString;
    }
    else {
        cerr << "(Unknown CUDA status code " << status << ")";
    }
    if (filename != NULL) {

        cerr << filename << ":" << line_number;
    }

    cerr << std::endl;
    if(die_on_error) {
        exit(EXIT_FAILURE);
            // ... or cerr << "FATAL ERROR" << etc. etc.
    }
    return false;
}

void printCudaMemory(char* info) {
    size_t free_byte ;
    size_t total_byte ;
    cudaError_t cuda_status = cudaMemGetInfo( &free_byte, &total_byte ) ;
    if ( cudaSuccess != cuda_status ){
        printf("Error: cudaMemGetInfo fails, %s \n", cudaGetErrorString(cuda_status) );
        exit(1);
    }

    double free_db = (double)free_byte ;
    double total_db = (double)total_byte ;
    double used_db = total_db - free_db ;
    printf("[%s]GPU memory usage: used = %f, free = %f MB, total = %f MB\n", info,
        used_db/1024.0/1024.0, free_db/1024.0/1024.0, total_db/1024.0/1024.0);
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

void splitStringBySpace(string input, int * output, int nCols){
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

void setBitHost(unsigned char *A, unsigned long long int k)
{
  A[k/8] |= 1 << (k%8);
}

__device__
void setBit(unsigned char *A, unsigned long long int k)
{
  A[k/8] |= 1 << (k%8);
}

short getBit(unsigned char *A, unsigned long long int k)
{
  return ( (A[k/8] & (1 << (k%8) )) != 0 ) ;
}

void readFileAndReturnTerrain(Terrain *h_terrain, char* file_name, int towerHeight, char* pointHeighMappingFilePath){
    ifstream fin;
    fin.open(file_name);
    // assume the file is in AAIGrid format
    string line_ncols, line_nrows, line_xllcorner, line_yllcorner, line_dx,line_dy, line_nodata;
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
    float xllcorner = atof(breakStringByspace(line_xllcorner).c_str());
    float yllcorner = atof(breakStringByspace(line_yllcorner).c_str());

    // for easier calculations later on, make sure we are dealing only with an even grid size (ignore the last row and column)
    h_terrain->nCols = nCols; //% 2 == 0 ? nCols : nCols-1;
    h_terrain->nRows = nRows; //% 2 == 0 ? nRows : nRows-1;
    h_terrain->xllcorner = xllcorner;
    h_terrain->yllcorner = yllcorner;
    if(line_nodata.find("NODATA_value") == string::npos){
        h_terrain->noDataValue = 65535;
    }else{
        h_terrain->noDataValue = atoi(breakStringByspace(line_nodata).c_str());
    }
    h_terrain->gridTerrian = (Pixel*) malloc((h_terrain->nRows * h_terrain->nCols) * sizeof(Pixel));
    int rowIndex = 0;
    int * pixelInRow = (int*) malloc(h_terrain->nCols * sizeof(int));
    // loop through the file and break when finish or nRow-1 is reached
    if(line_nodata.find("NODATA_value") == string::npos){
        splitStringBySpace(line_nodata, pixelInRow, h_terrain->nCols);
        for(int j = 0; j < h_terrain->nCols; j++){
            h_terrain->gridTerrian[(h_terrain->nCols * rowIndex) + j].elevation = pixelInRow[j];
        }
        rowIndex++;
    }
    while(getline(fin, line_ncols) && rowIndex != h_terrain->nRows){
        splitStringBySpace(line_ncols, pixelInRow, h_terrain->nCols);
        for(int j = 0; j < h_terrain->nCols; j++){
            h_terrain->gridTerrian[(h_terrain->nCols * rowIndex) + j].elevation = pixelInRow[j];
        }
        rowIndex++;
    }
    free(pixelInRow);

    /*
     * Print the grid for testing
     *
     for(int i = 0; i < h_terrain->nRows; i++){
    	for(int j = 0; j < h_terrain->nCols; j++){
    		cout << h_terrain->gridTerrian[i * h_terrain->nRows + j].elevation << " ";
    	}
    	cout << endl;
      }
      exit(1);
    *
    *
    */
    /////
    ifstream pointHeighMappingFile(pointHeighMappingFilePath);
    map<long, int> towerHeightMap;
    string line;long towerIndex; int height;
    while (pointHeighMappingFile >> line){
        stringstream ss(line);
        string token;
        getline(ss, token, ',');
        towerIndex = stol(token);
        getline(ss, token, ',');
        height = stoi(token);
        towerHeightMap[towerIndex] = height;
    }
    pointHeighMappingFile.close();
    map<long, int>::iterator it;
    for(it = towerHeightMap.begin(); it != towerHeightMap.end(); ++it){
        std::cout << it->first << " => " << it->second << '\n';
    }

    /////
    h_terrain->towerLocations = (Point*) malloc((h_terrain->nRows * h_terrain->nCols)  * sizeof(Point));
    towerIndex = 0;

    for(int i = 0; i < h_terrain->nRows; i++){
        for(int j = 0; j < h_terrain->nCols; j++){
            // cout << h_terrain->gridTerrian[h_terrain->nCols * i + j].elevation << " ";
            if(towerHeightMap.count((h_terrain->nCols * i) + j) > 0){
                h_terrain->gridTerrian[(h_terrain->nCols * i) + j].mountingHeight = towerHeightMap[(h_terrain->nCols * i) + j];
            }else{
                h_terrain->gridTerrian[(h_terrain->nCols * i) + j].mountingHeight = towerHeight;
            }
            h_terrain->towerLocations[towerIndex].row = i;
            h_terrain->towerLocations[towerIndex].col = j;
            towerIndex++;
        }
        // cout << endl;
    }

    //testing of mounting height
    // for(int i = 0; i < h_terrain->nRows; i++){
    //     for(int j = 0; j < h_terrain->nCols; j++){
    //         cout << "Height of tower " << (h_terrain->nCols * i) + j << " is: " << h_terrain->gridTerrian[(h_terrain->nCols * i) + j].mountingHeight << endl;
    //     }
    // }


    //For testing tower position
    //cout << h_terrain->towerLocations[7].row << h_terrain->towerLocations[7].col << endl;
    //cout << h_terrain->gridTerrian[(h_terrain->nCols *  h_terrain->towerLocations[7].row) + h_terrain->towerLocations[7].col].elevation

    fin.close();

}

__device__
double calculateGradientOnLine(Pixel *inputTerrain, Point *observer, Point *target, int width){
    double distanceFromObserverToTarget = sqrtf(powf( (float) (target->row - observer->row) , 2.0) + powf( (float) (target->col - observer->col) , 2.0));
    int gridIndexTarget = (width * target->row) + target->col;
    int gridIndexObserver = (width * observer->row) + observer->col;
    double targetTotalElevation = inputTerrain[gridIndexTarget].elevation + inputTerrain[gridIndexTarget].mountingHeight;
    double observerTotalElevation = inputTerrain[gridIndexObserver].elevation + inputTerrain[gridIndexObserver].mountingHeight;
    return (targetTotalElevation - observerTotalElevation) / distanceFromObserverToTarget;
}

__device__
double calculateGradientOnLineWithoutTowerHeight(Pixel *inputTerrain, Point *observer, Point *target, int width){
    double distanceFromObserverToTarget = sqrtf(powf( (float) (target->row - observer->row) , 2.0) + powf( (float) (target->col - observer->col) , 2.0));
    int gridIndexTarget = (width * target->row) + target->col;
    int gridIndexObserver = (width * observer->row) + observer->col;
    double targetTotalElevation = inputTerrain[gridIndexTarget].elevation;
    double observerTotalElevation = inputTerrain[gridIndexObserver].elevation + inputTerrain[gridIndexObserver].mountingHeight;
    return (targetTotalElevation - observerTotalElevation) / distanceFromObserverToTarget;
}

int getBoundaryAroundObserver(int nRows, int nCols, Point *observerBoundary){

    int topRow = 0;
    int bottomRow = nRows-1;
    int leftCol = 0;
    int rhtCol = nCols-1;

    //storing boundary points around observer
    int observerBoundaryIndex = 0;
    for(int i = leftCol; i <= rhtCol; i = i + STEPSIZE ){
        Point p;
        p.row = topRow;
        p.col = i;
        observerBoundary[observerBoundaryIndex] = p;
        observerBoundaryIndex++;
    }
    for(int i = topRow+1; i <= bottomRow; i = i + STEPSIZE){
        Point p;
        p.row = i;
        p.col = rhtCol;
        observerBoundary[observerBoundaryIndex] = p;
        observerBoundaryIndex++;
    }
    for(int i = rhtCol-1; i >= leftCol; i = i - STEPSIZE){
        Point p;
        p.row = bottomRow;
        p.col = i;
        observerBoundary[observerBoundaryIndex] = p;
        observerBoundaryIndex++;
    }
    for(int i = bottomRow-1; i > topRow; i = i - STEPSIZE){
        Point p;
        p.row = i;
        p.col = leftCol;
        observerBoundary[observerBoundaryIndex] = p;
        observerBoundaryIndex++;
    }
    return observerBoundaryIndex;
}

__device__
void my_swap(float &x, float &y){
	float temp = 0.0;
	temp = x;
	x = y;
	y = temp;
}

__device__
int getPointsOnLine(Point *start, Point *end, Point *allPointsInLine, short nCols){
	// Bresenham's line algorithm
	 float x1 = start->row, y1 = start->col, x2 = end->row, y2 = end->col; //CHECK: these values are actually short
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
	    if(steep)
	    {
	    	allPointsInLine[steps].row = (int)y;
	    	allPointsInLine[steps].col = (int)x;
	    }
	    else
	    {
	    	allPointsInLine[steps].row = (int)x;
	    	allPointsInLine[steps].col = (int)y;
	    }
	    error -= dy;
	    if(error < 0)
	    {
	        y += ystep;
	        error += dx;
	    }
	    steps++;
	  }
	  return steps;
}

__global__
void calculateViewshed(Terrain terrain, unsigned char* d_viewshed, Point *viewshedBoundary, int boundarySize, int size, int iter, int r, Point* d_pointsOnLine, short sizeOfLine){

    unsigned long long int observerIndex = blockDim.x * blockIdx.x + threadIdx.x;
    // unsigned long long int yIndex = blockDim.y * blockIdx.y + threadIdx.y;
    observerIndex = observerIndex + iter;

    if (observerIndex >= iter + r){
        // printf("exiting %llu\n", observerIndex);
        return;
    }
    // printf("%llu\n", observerIndex);


    unsigned long long int viewshedIndex = terrain.nRows * terrain.nCols * (observerIndex - iter);
    Point observerPoint = terrain.towerLocations[observerIndex];

    //skip no data values
    if(terrain.gridTerrian[observerPoint.row * terrain.nCols + observerPoint.col].elevation == terrain.noDataValue){
    	return;
    }

    for(int i = 0; i < boundarySize; i++){
         int stepsOnLine = getPointsOnLine(&observerPoint, &viewshedBoundary[i], &d_pointsOnLine[(observerIndex - iter) * sizeOfLine], terrain.nCols);
         double maxGradientBetweenObserverAndTarget = PTRDIFF_MIN;

    	//for(int j = stepsOnLine - 1; j >= 0; j--){
        for(int j = 1; j < stepsOnLine; j++){
    		unsigned long long int grid_index = (terrain.nCols * d_pointsOnLine[(observerIndex - iter) * sizeOfLine +j].row) + d_pointsOnLine[(observerIndex - iter) * sizeOfLine +j].col;
    		if(terrain.gridTerrian[grid_index].elevation != terrain.noDataValue){
    		    double gradient = calculateGradientOnLine(terrain.gridTerrian, &observerPoint, &d_pointsOnLine[(observerIndex - iter) * sizeOfLine +j], terrain.nCols);
                double gradientWithoutTowerHeight = calculateGradientOnLineWithoutTowerHeight(terrain.gridTerrian, &observerPoint, &d_pointsOnLine[(observerIndex - iter) * sizeOfLine +j], terrain.nCols);
                // if(observerIndex == 0 && i == 19){
                //     printf("%d,%f,%f\n",j, gradient, gradientWithoutTowerHeight);
                // }
                if(gradient >= maxGradientBetweenObserverAndTarget){
    		    	setBit(d_viewshed, viewshedIndex + grid_index);

    		        maxGradientBetweenObserverAndTarget = gradientWithoutTowerHeight;
                    // maxGradientBetweenObserverAndTarget = gradient;
    		    }
    		}
    	}
         //calculating for the last point
        unsigned long long int grid_index = (terrain.nCols * viewshedBoundary[i].row) + viewshedBoundary[i].col;
        if(terrain.gridTerrian[grid_index].elevation != terrain.noDataValue){
            double gradient = calculateGradientOnLine(terrain.gridTerrian, &observerPoint, &viewshedBoundary[i], terrain.nCols);
            double gradientWithoutTowerHeight = calculateGradientOnLineWithoutTowerHeight(terrain.gridTerrian, &observerPoint, &viewshedBoundary[i], terrain.nCols);
            if(gradient >= maxGradientBetweenObserverAndTarget){
                setBit(d_viewshed, viewshedIndex + grid_index);
                maxGradientBetweenObserverAndTarget = gradientWithoutTowerHeight;
            }
        }

    }

}

int getIndexOfTower(Point *towerLocations, int size, int row, int col){
    int index = -1;
    for(int i = 0; i < size; i++){
        if(towerLocations[i].row == row && towerLocations[i].col == col){
            index = i;
        }
    }
    return index;
}

int main(int argc, char* argv[]){
    int dev = 1;
    CUDA_CALL(cudaSetDevice, dev);
    cudaError_t err = cudaSuccess;
    Terrain h_terrain;
    Pixel *d_pixel_grid, *h_pixel_grid; Point *d_towers, *h_towers;

    if(argc < 2){
        cout << "Not enough arguments!" << endl;
        cout << "example: ./main /path/to/file" << endl;
        return 1;
    }
    cout << "Reading Input File...." << endl;
    readFileAndReturnTerrain(&h_terrain, argv[1], atoi(argv[3]), argv[5]);
    //exit(1);
    cout << "After file output" << endl;
    h_pixel_grid = h_terrain.gridTerrian;
    h_towers = h_terrain.towerLocations;

    int THREADS = atoi(argv[2]); //4

    unsigned long long int totalThreads = THREADS;
    cout << "totalThreads: " << totalThreads << endl;
    cout << "nRows: " << h_terrain.nRows << ", nCols: " << h_terrain.nCols << endl;
    float lenGlobal = (float)(h_terrain.nRows*h_terrain.nCols)/8;
    std::cout << "MemLength " << lenGlobal<< std::endl;
    printCudaMemory((char*)"1");
    CUDA_CALL(cudaMalloc, (void**)&d_pixel_grid, (h_terrain.nRows * h_terrain.nCols) * sizeof(Pixel));
    CUDA_CALL(cudaMemcpy, d_pixel_grid, h_pixel_grid, (h_terrain.nRows * h_terrain.nCols) * sizeof(Pixel), cudaMemcpyHostToDevice);
    printCudaMemory((char*)"2");
    CUDA_CALL(cudaMalloc, (void**)&d_towers, totalThreads * sizeof(Point));
    CUDA_CALL(cudaMemcpy, d_towers, h_towers, totalThreads * sizeof(Point), cudaMemcpyHostToDevice);
    printCudaMemory((char*)"3");
    h_terrain.towerLocations = d_towers;
    h_terrain.gridTerrian = d_pixel_grid;

    int boundarySize = (h_terrain.nCols + h_terrain.nRows) / STEPSIZE;
    Point *h_viewshedBoundary, *d_viewshedBoundary;
    h_viewshedBoundary = (Point*) malloc( ((boundarySize) * 2) * sizeof(Point));
    int totalBoundarySize = getBoundaryAroundObserver(h_terrain.nRows, h_terrain.nCols, h_viewshedBoundary);
    CUDA_CALL(cudaMalloc, (void**)&d_viewshedBoundary, ((boundarySize) * 2) * sizeof(Point));
    CUDA_CALL(cudaMemcpy, d_viewshedBoundary, h_viewshedBoundary, ((boundarySize ) * 2) * sizeof(Point), cudaMemcpyHostToDevice);
    printCudaMemory((char*)"4");
    cout << "Total Boundary Size: " << totalBoundarySize << endl;
    cout << "Starting Kernel with # of threads " << totalThreads<<endl;
    size_t heap;
    CUDA_CALL(cudaDeviceGetLimit, &heap, cudaLimitMallocHeapSize);
    cout << "Heap size before = " << heap << endl;
    // this is dirty fix
    // CUDA_CALL(cudaDeviceSetLimit, cudaLimitMallocHeapSize, heap*max(1,THREADS/1000));
    // CUDA_CALL(cudaDeviceGetLimit, &heap, cudaLimitMallocHeapSize);
    cout << "Heap size after = " << heap << endl;

    cudaDeviceProp myCUDA;
    if (cudaGetDeviceProperties(&myCUDA, dev) == cudaSuccess)
    {
        printf("Using device %d:\n", dev);
        printf("%s; global mem: %zdByte; compute v%d.%d; clock: %d kHz\n",
            myCUDA.name, myCUDA.totalGlobalMem, (int)myCUDA.major,
            (int)myCUDA.minor, (int)myCUDA.clockRate);
    }

    int threadsPerBlock = myCUDA.maxThreadsPerBlock;
    int blocksPerGrid = (totalThreads + threadsPerBlock - 1) / threadsPerBlock;
    cout << "Maximum threads per block = " << threadsPerBlock << endl;
    cout << "Blocks per Grid = " << blocksPerGrid << endl;
    cout << "Size of global viewshed" << ceil(totalThreads * lenGlobal *sizeof(unsigned char)) << endl;
    unsigned char *g_viewshed = (unsigned char *) calloc(ceil(totalThreads * lenGlobal *sizeof(unsigned char)), sizeof(unsigned char));
    if (g_viewshed == NULL)
    {
        std::cout << "Memory allocation failed" << std::endl;
        exit(1);
    }
    unsigned long long int MAX_ELEMENTS = 250000UL * 250000UL; //25600000000; // 500x500

    int r = max(int(MAX_ELEMENTS / totalThreads), 1); //make sure we process at least 1 row
    printf("Rows to process in each iteration %d\n", r);
    printf("Iterations %f\n", ceil(totalThreads * totalThreads / MAX_ELEMENTS));

    for (int i = 0; i < totalThreads; i+=r) { //step

        float len = (float)(totalThreads)/8;
        printf("Start Iteration %d, %f , %f\n", i/r,len,ceil(r * len *sizeof(unsigned char)) );
        // std::cout << r << " " << len << " " << ceil(r * len *sizeof(unsigned char)) << std::endl;

        // we are in the last thread, only process the remaining viewsheds
        if (i+r > totalThreads) {
            r = totalThreads - i;
            // std::cout << "Remainder " << r << std::endl;
        }
        unsigned char *h_viewshed, *d_viewshed;

        // round up to the next full byte
        h_viewshed = (unsigned char *) calloc(ceil(r * len *sizeof(unsigned char)), sizeof(unsigned char));
        // cout << "viewshed size " << r * len *sizeof(unsigned char) * 8 << " elements --> allocated memory: " << ceil(r * len *sizeof(unsigned char)) << " bytes" << endl;
        // std::cout << "MemLength~~ " <<ceil(r * len *sizeof(unsigned char)) << std::endl;
        CUDA_CALL(cudaMalloc, (unsigned char**)&d_viewshed, (ceil(r * len *sizeof(unsigned char))));
        CUDA_CALL(cudaMemcpy, d_viewshed, h_viewshed, ceil(r* len *sizeof(unsigned char)), cudaMemcpyHostToDevice);
        printCudaMemory((char*)"Viewshed");

        // allocate memory for the max number of possible points, i.e. map size
        int sizeOfLine = sqrt(pow(h_terrain.nRows , 2.0) + pow(h_terrain.nCols, 2.0) );

        Point *h_pointsOnLine,*d_pointsOnLine;
        h_pointsOnLine = (Point*)malloc(r * sizeOfLine * sizeof(Point)); //FIXME
        CUDA_CALL(cudaMalloc, (Point **)&d_pointsOnLine, (r * sizeOfLine * sizeof(Point)));
        CUDA_CALL(cudaMemcpy, d_pointsOnLine, h_pointsOnLine, r* sizeOfLine *sizeof(Point), cudaMemcpyHostToDevice);
        printCudaMemory((char*)"Viewshed");

        std::cout << "Iteration Start" << std::endl;
        Print_Time();

        // calculateViewshed<<<blocksPerGrid, threadsPerBlock>>>(h_terrain, d_viewshed, d_viewshedBoundary, totalBoundarySize, totalThreads, i, r);
        calculateViewshed<<<blocksPerGrid, threadsPerBlock>>>(h_terrain, d_viewshed, d_viewshedBoundary, totalBoundarySize, totalThreads, i, r, d_pointsOnLine, sizeOfLine);

        cudaError_t errSync  = cudaGetLastError();
        if (errSync != cudaSuccess) {
            printf("Sync kernel error: %s\n", cudaGetErrorString(errSync));
            exit(EXIT_FAILURE);
        }
        cout << "Waiting for all jobs to finish..." << endl;
        cudaError_t errAsync = cudaDeviceSynchronize();
        if (errAsync != cudaSuccess) {
            printf("Async kernel error: %s\n", cudaGetErrorString(errAsync));
            exit(EXIT_FAILURE);
        }

        std::cout << "Iteration stop" << std::endl;
        Print_Time();

        cout << "Copy viewsheds from device to host" << endl;
        err = cudaMemcpy(h_viewshed, d_viewshed, ceil(r* len *sizeof(unsigned char)), cudaMemcpyDeviceToHost);
        if (err != cudaSuccess){
            cout << "Failed to copy viewsheds grid from device to host. Error String " << cudaGetErrorString(err) << err << endl;
            exit(EXIT_FAILURE);
        }
        cout << "Viewshed calculation complete" << endl;
        // Print_Time();

        // memcpy(g_viewshed + (r * len * sizeof(unsigned char)), h_viewshed, ceil(r* len *sizeof(unsigned char)));

        // exit(1);

         // commenting it due to seg fault after iteration 1
        unsigned long long int count = 0;
        for(unsigned long long int k = (i * totalThreads); k < ( (unsigned long long int)(i * totalThreads) + (r * totalThreads) ); k++){
            // std::cout << k << " " <<getBit(h_viewshed, count)<< std::endl;
            // g_viewshed.set(k, getBit(h_viewshed, count));
            if(getBit(h_viewshed, count)){
                // std::cout << k << std::endl;
                setBitHost(g_viewshed,k);
            }
            count++;
            // g_viewshed[k] = getBit(h_viewshed, j * (h_terrain.nRows * h_terrain.nCols) + k)
        }


        // FILE *fp;
        // char buff[10];
        // sprintf(buff,"%d",i);
        // fp = fopen(buff, "wb");
        // // long tRows = (h_terrain.nRows * h_terrain.nCols);
        // for(int j = r-1; j < r; j++){
        //     for(int k = 0; k < totalThreads; k++){
        //         // unsigned long long int index = ((h_terrain.nRows * h_terrain.nCols) * (unsigned long long int) indexOfTower) + ((h_terrain.nCols * j) + k);
        //         unsigned long long int mainIndex = j * totalThreads + k;
        //         if(getBit(h_viewshed, mainIndex)){
        //             // cout << "1 " << mainIndex << endl;
        //             fprintf(fp, "1 ");
        //         }
        //         else{
        //             // cout << "0 " << mainIndex << endl;
        //             fprintf(fp, "0 ");
        //         }
        //     }
        //     fprintf(fp, "\n");
        // }
        // fclose(fp);

        free(h_viewshed);
        free(h_pointsOnLine);
        cudaFree(d_pointsOnLine);
        cudaFree(d_viewshed);
    }

    // std::cout << "Writing to file..." << std::endl;
    // FILE *fp;
    // fp = fopen("out.txt", "wb");

    // for(int j = 0; j < totalThreads; j++){
    //     for(int k = 0; k < totalThreads; k++){
    //         // unsigned long long int index = ((h_terrain.nRows * h_terrain.nCols) * (unsigned long long int) indexOfTower) + ((h_terrain.nCols * j) + k);
    //         if(getBit(g_viewshed, j * (h_terrain.nRows * h_terrain.nCols) + k)){
    //             fprintf(fp, "1 ");
    //         }
    //         else
    //             fprintf(fp, "0 ");
    //     }
    //     fprintf(fp, "\n");
    // }
    // fclose(fp);

    // for(int j = 0; j < totalThreads; j++){
    //         for(int k = 0; k < totalThreads; k++){
    //             // unsigned long long int index = ((h_terrain.nRows * h_terrain.nCols) * (unsigned long long int) indexOfTower) + ((h_terrain.nCols * j) + k);
    //             if(getBit(g_viewshed, j * (h_terrain.nRows * h_terrain.nCols) + k)){
    //                 cout << "1 ";
    //             }
    //             else{
    //                 cout << "0 ";
    //             }
    //         }
    //         cout << endl;
    //     }
    std::cout << "Writing to binary file..." << std::endl;
    FILE * write_ptr;
    write_ptr = fopen(argv[4],"wb");
    fwrite(g_viewshed, sizeof(unsigned char), ceil(totalThreads * lenGlobal), write_ptr);
    fclose(write_ptr);

    free(g_viewshed);

    cudaFree(d_pixel_grid);
    cudaFree(d_towers);
    cudaFree(d_viewshedBoundary);
    free(h_pixel_grid);
    free(h_towers);
    free(h_viewshedBoundary);
    // reset the device
    err = cudaDeviceReset();
    return 0;
}
