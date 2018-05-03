//
// Created by cuda on 2/25/18.
//


#include "Graph.h"

Graph::Graph(long V) {
    this->V = V;
    this->adj = new list<iPair> [V];

}

void Graph::addEdge(long u, long v, double w) {
    this->adj[u].push_back(make_pair(v, w));
    this->adj[v].push_back(make_pair(u, w));
}

list< pair<long, double> >* Graph::getGraph() {
    return this->adj;
}

void Graph::print() {
    for(long i = 0; i < this->V; i++){
        cout << "From: " << i << endl;
        for (list<iPair>::iterator it = this->adj[i].begin(); it != this->adj[i].end(); ++it){
            cout << "\t" << it->first << endl;
        }
    }
}

void Graph::printPath(vector<long> parent, long to, vector<long>& pathList) {
    // Base Case : If j is source
    if (parent[to] == -1)
        return;

    printPath(parent, parent[to], pathList);
//    printf("%li ", to);
    pathList.push_back(to);
}

shortestPathResult Graph::shortestPath(long src) {
    shortestPathResult shortestPathResult1;
    // Create a priority queue to store vertices that
    // are being preprocessed. This is weird syntax in C++.
    // Refer below link for details of this syntax
    // http://geeksquiz.com/implement-min-heap-using-stl/

    priority_queue< iPair, vector <iPair> , greater<iPair> > pq;

    // Create a vector for distances and initialize all
    // distances as infinite (INF)
    vector<double > dist(this->V, DBL_MAX);
    vector<long> parent(this->V, -1);
    // Insert source itself in priority queue and initialize
    // its distance as 0.
    pq.push(make_pair(0, src));
    dist[src] = 0;
    parent[src] = -1;
    /* Looping till priority queue becomes empty (or all
      distances are not finalized) */
    while (!pq.empty())
    {
        // The first vertex in pair is the minimum distance
        // vertex, extract it from priority queue.
        // vertex label is stored in second of pair (it
        // has to be done this way to keep the vertices
        // sorted distance (distance must be first item
        // in pair)
        int u = pq.top().second;
        pq.pop();

        // 'i' is used to get all adjacent vertices of a vertex
        list< pair<long, double> >::iterator i;
        for (i = adj[u].begin(); i != adj[u].end(); ++i)
        {
            // Get vertex label and weight of current adjacent
            // of u.
            long v = (*i).first;
            double weight = (*i).second;

            //  If there is shorted path to v through u.
            if (dist[v] > dist[u] + weight)
            {
                // Updating distance of v
                dist[v] = dist[u] + weight;
                parent[v] = u;
                pq.push(make_pair(dist[v], v));
            }
        }
    }
    double maxDistance = DBL_MIN;
    long toNode = -1;
    // Print shortest distances stored in dist[]
//    printf("Vertex   Distance from Source\n");

    for (int i = 0; i < V; ++i) {
        if(dist[i] != DBL_MAX && dist[i] > maxDistance){
            maxDistance = dist[i];
            toNode = i;
        }
    }
    vector<long> pathList;
    if(toNode != -1){
        printPath(parent, toNode, pathList);
    }
    shortestPathResult1.source = src;
    shortestPathResult1.to = toNode;
    shortestPathResult1.dist = maxDistance;
    shortestPathResult1.hops = pathList.size();
    return shortestPathResult1;
}

void Graph::shortestPathWithMatrix(long src, double** distanceMatrix) {

    priority_queue< iPair, vector <iPair> , greater<iPair> > pq;

    vector<double > dist(this->V, DBL_MAX);
    // Insert source itself in priority queue and initialize
    // its distance as 0.
    pq.push(make_pair(0, src));
    dist[src] = 0;

    while (!pq.empty())
    {
        int u = pq.top().second;
        pq.pop();

        list< pair<long, double> >::iterator i;
        for (i = adj[u].begin(); i != adj[u].end(); ++i)
        {
            long v = (*i).first;
            double weight = (*i).second;

            //  If there is shorted path to v through u.
            if (dist[v] > dist[u] + weight)
            {
                // Updating distance of v
                dist[v] = dist[u] + weight;
                pq.push(make_pair(dist[v], v));
            }
        }
    }
//    printf("Vertex   Distance from Source: %li\n", src);
    for (int i = 0; i < V; ++i) {
        distanceMatrix[src][i] = dist[i];
    }
}



long Graph::BFS(long s) {

    bool *visited = new bool[V];
    long visitedCounter = 0;
    for(long i = 0; i < V; i++)
        visited[i] = false;

    // Create a queue for BFS
    list<long> queue;

    // Mark the current node as visited and enqueue it
    visited[s] = true;

    visitedCounter++;
    queue.push_back(s);

    // 'i' will be used to get all adjacent
    // vertices of a vertex
    list< pair<long, double> >::iterator i;

    while(!queue.empty())
    {
        // Dequeue a vertex from queue and print it
        s = queue.front();
//        cout << s << " ";
        queue.pop_front();

        // Get all adjacent vertices of the dequeued
        // vertex s. If a adjacent has not been visited,
        // then mark it visited and enqueue it
        for (i = adj[s].begin(); i != adj[s].end(); ++i)
        {
            if (!visited[(*i).first])
            {
                visited[(*i).first] = true;

                visitedCounter++;
                queue.push_back((*i).first);
            }
        }
    }
    cout << endl;
//    cout << "Node Visited From: " << originalSource << " are: " << visitedCounter << endl;
    delete [] visited;
    return visitedCounter;
}

long Graph::BFSV2(long s, bool *globalVisitedNodes, Coordinate *indexMapping, Coordinate *remainingPointsList, char* outputTreeFilePath) {
    ofstream outputTreeFileHandel(outputTreeFilePath);
    outputTreeFileHandel.precision(15);
    int treePathCounterLocal = 1;
//    bool *visited = new bool[V];
    long visitedCounter = 0;
//    for(long i = 0; i < V; i++)
//        visited[i] = false;

    // Create a queue for BFS
    list<long> queue;

    // Mark the current node as visited and enqueue it
//    visited[s] = true;
    globalVisitedNodes[s] = true;
    visitedCounter++;
    queue.push_back(s);

    // 'i' will be used to get all adjacent
    // vertices of a vertex
    list< pair<long, double> >::iterator i;

    while(!queue.empty())
    {
        // Dequeue a vertex from queue and print it
        s = queue.front();
//        cout << s << " ";
        queue.pop_front();

        // Get all adjacent vertices of the dequeued
        // vertex s. If a adjacent has not been visited,
        // then mark it visited and enqueue it
        for (i = adj[s].begin(); i != adj[s].end(); ++i)
        {
            if (!globalVisitedNodes[(*i).first])
            {
//                visited[(*i).first] = true;
                globalVisitedNodes[(*i).first] = true;
                outputTreeFileHandel << "L 2 1" << endl;
                outputTreeFileHandel << indexMapping[ remainingPointsList[s].index ].lng << " " << indexMapping[ remainingPointsList[s].index ].lat << endl;
                outputTreeFileHandel << indexMapping[ remainingPointsList[(*i).first].index ].lng << " " << indexMapping[ remainingPointsList[(*i).first].index ].lat << endl;
                outputTreeFileHandel << 1 << " " << treePathCounterLocal << endl;
                treePathCounterLocal++;
                visitedCounter++;
                queue.push_back((*i).first);
            }
        }
    }
    outputTreeFileHandel.close();
    cout << endl;
//    cout << "Node Visited From: " << originalSource << " are: " << visitedCounter << endl;
//    delete [] visited;
    return visitedCounter;
}

Graph::~Graph() {
//    printf("Destructor Called\n");
    delete []this->adj;
}