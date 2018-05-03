#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <cstring>
#include <vector>
using namespace std;

short getBit(unsigned char *A, unsigned long long int k) {
    return ( (A[k/8] & (1 << (k%8) )) != 0 ) ;
}
void setBit(unsigned char *A, unsigned long long int k) {
    A[k/8] |= 1 << (k%8);
}
// example graph
bool viewsheds[81] = {1, 1, 1, 0, 0, 0, 0, 0, 0,
                      1, 1, 0, 1, 1, 0, 0, 0, 1,
                      1, 0, 1, 0, 1, 0, 0, 1, 0,
                      0, 1, 0, 1, 0, 0, 0, 0, 0,
                      0, 1, 1, 0, 1, 1, 1, 0, 1,
                      0, 0, 0, 0, 1, 1, 1, 0, 0,
                      0, 0, 0, 0, 1, 1, 1, 0, 0,
                      0, 0, 1, 0, 0, 0, 0, 1, 0,
                      0, 1, 0, 0, 1, 0, 0, 0, 1};

long rows = 40000;

void create_binary_file() {
    FILE *write_ptr;
    unsigned char *miniMatrix = (unsigned char *) calloc(ceil((float)sizeof(viewsheds)/8) * sizeof(unsigned char), sizeof(unsigned char));
    for (int i = 0; i < sizeof(viewsheds); ++i) {
        if (viewsheds[i]) {
            setBit(miniMatrix,i);
        }
    }

    write_ptr = fopen("mini_matrix.bin","wb");

    fwrite(miniMatrix, sizeof(unsigned char), ceil((float)sizeof(viewsheds)/8) * sizeof(unsigned char), write_ptr);
    fclose(write_ptr);
    free(miniMatrix);
}

int main(int argc, char *argv[]) {
    rows = stoi(argv[2]) * stoi(argv[3]);

    srand(8452);

    // create_binary_file();

    unsigned char *visibilityMatrix;
    // FILE* fstr = fopen("mini_matrix.bin", "rb");

    // FILE* fstr = fopen("/home/thp/Dropbox/WISP_planning/sharing/200by200/out.bin", "rb");
    FILE* fstr = fopen(argv[4], "rb");

    visibilityMatrix = (unsigned char *) calloc(ceil((float)(rows *rows)/8) * sizeof(unsigned char), sizeof(unsigned char));
    fread(visibilityMatrix, sizeof(unsigned char), ceil((float)(rows *rows)/8) * sizeof(unsigned char), fstr);
    fclose(fstr);

    std::vector<int> randomRows;
    ///
    bool noDataPoints[rows] = {false};
    ifstream noDataPointsHandler("");
    int nodataPointIndex = -1;
    int totalNoDataPoints = 0;
    while(noDataPointsHandler >> nodataPointIndex){
        noDataPoints[nodataPointIndex] = true;
        totalNoDataPoints++;
    }
    cout << "Total No Data Values: " << totalNoDataPoints << endl;
    noDataPointsHandler.close();
    ///

    int* randomArray = new int[rows];
    for (int i = 0; i < rows; ++i) {
        randomArray[i] = 0;
    }

    for (int i = 0; i < rows; ++i) {
        if(!noDataPoints[i]){
            randomRows.push_back(i);
        }
    }

    random_shuffle ( randomRows.begin(), randomRows.end() );

    // for (std::vector<int>::iterator it=randomRows.begin(); it!=randomRows.end(); ++it) {
    //     std::cout << ' ' << *it;
    // }

    // for (int i = 0; i < rows; ++i) {
    //     std::cout << randomArray[i] << std::endl;
    // }

    // std::cout  << std::endl;
    // cout <<randomRows.size() <<endl;
    int lastIdx = randomRows.size() * stof(argv[1])/100;
    // std::cout << "Muting " << lastIdx << " rows:"<<endl;;
    ofstream mutedPointsFile(argv[5]);

    std::vector<int> randomRowsSub(&randomRows[0], &randomRows[lastIdx]);
    for (std::vector<int>::iterator it=randomRowsSub.begin(); it!=randomRowsSub.end(); ++it) {
        randomArray[*it] = 1;
        mutedPointsFile << *it << endl;
        // std::cout << ' ' << *it;
    }
    mutedPointsFile.close();
    // std::cout  << std::endl;
    // std::cout << "edges: " << edges << std::endl;
    char appendedNewName[15] = "Remaining.txt\0";
    char nonRemovedFilePath[300] = {'\0'};
    strcpy(nonRemovedFilePath, argv[5]);
    nonRemovedFilePath[strlen(nonRemovedFilePath)-4] = '\0';
    strcat(nonRemovedFilePath, appendedNewName);

    ofstream nonRemovedFileHandel(nonRemovedFilePath);
    int reminingNodes = 0;
    for(int i = 0; i < rows; i++){
        if(randomArray[i] == 0 && !noDataPoints[i]){
            nonRemovedFileHandel << i << endl;
            reminingNodes++;
        }
    }
    nonRemovedFileHandel.close();
    // cout << reminingNodes << endl;

    // int connected_nodes = 0;
    // for (int j = 0; j < rows; ++j) {
    //     for (int i = 0; i < rows; ++i) {
    //         if (randomArray[i]) {
    //             continue;
    //         }
    //         if (i == j)
    //             continue;
    //         if (getBit(visibilityMatrix, i*rows+j)) {
    //             connected_nodes ++;
    //             break;
    //         }
    //     }
    // }

    // int noDataPointCon = 0;
    // for(int i = 0; i < rows; i++){
    //     for(int j = 0; j < rows; j++){
    //         if(randomArray[i] == 0 && !noDataPoints[i]){
    //             if (randomArray[j] == 0 && !noDataPoints[j] && i != j && getBit(visibilityMatrix, i*rows+j)) {
    //                 //scout << i << ", " << j << endl;
    //                 noDataPointCon ++;
    //                 break;
    //             }
    //         }
    //     }
    // }
    // don't divide by rows here as there might be rows with all zeros, excluded by cuda viewshed calculation (NULL)
    // std::cout <<connected_nodes*100./rows << std::endl;
    // int totalMutedPoints = rows - totalNoDataPoints - reminingNodes;
    // double percentOfTotalRemoval = ( (totalMutedPoints * 100.0) / (rows - totalNoDataPoints) );
    // std::cout << ( (double)connected_nodes / (double)(rows - totalNoDataPoints) ) * 100.0 << ","
    //           << ( (double)noDataPointCon / (double)reminingNodes ) * 100.0 << "," << percentOfTotalRemoval << std::endl;


    return 0;
}

// bool viewsheds[81] = {1, 1, 1, 0, 0, 0, 0, 0, 0,
//                       1, 1, 0, 1, 1, 0, 0, 0, 1,
//                       1, 0, 1, 0, 1, 0, 0, 1, 0,
//                       0, 1, 0, 1, 0, 0, 0, 0, 0,
//                       0, 1, 1, 0, 1, 1, 1, 0, 1,
//                       0, 0, 0, 0, 1, 1, 1, 0, 0,
//                       0, 0, 0, 0, 1, 1, 1, 0, 0,
//                       0, 0, 1, 0, 0, 0, 0, 1, 0,
//                       0, 1, 0, 0, 1, 0, 0, 0, 1};







// int j = 8;
// for (int i = 0; i < rows; ++i) {
//     if (i == j)
//         continue;
//     if (getBit(visibilityMatrix, i*rows+j)) {
//         std::cout << i << ", " << j << ", " << "match" << std::endl;

//     }
// }

// int from;
// std::string h = ";";
// while (true) {
//     getline(std::cin, h);
//     from = stoi(h);

//     std::cout << from/nodes << " " << from%nodes<< std::endl;
//     // arr from = stoi(h);ay[j/nodes][j%nodes]
// }
//
// Created by cuda on 2/21/18.
//

