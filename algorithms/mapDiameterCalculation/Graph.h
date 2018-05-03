//
// Created by cuda on 2/25/18.
//
#include <list>
#include <bits/stdc++.h>

//# define INF 0x3f3f3f3f
#ifndef NYU_PROJECT_GRAPH_H
#define NYU_PROJECT_GRAPH_H

using namespace std;


struct shortestPathResult{
    long source;
    long to;
    double  dist;
    int hops;
};

struct Coordinate{
    double lat;
    double lng;
    long index;
};


// iPair ==>  Integer Pair
typedef pair<long, double > iPair;

class Graph {
    long V;
    list< pair<long, double> > *adj;
public:

    void print();

    list< pair<long, double> >* getGraph();

    Graph(long V);  // Constructor

    // function to add an edge to graph
    void addEdge(long u, long v, double w);

    // prints shortest path from s
    shortestPathResult shortestPath(long s);

    //Breadth First Search
    long BFS(long s);
    long BFSV2(long s, bool *globalVisitedNodes, Coordinate *indexMapping, Coordinate *remainingPointsList, char* outputTreeFilePath);

    void printPath(vector<long> parent, long to,vector<long> &pathList);
    ~Graph();
};


#endif //NYU_PROJECT_GRAPH_H
