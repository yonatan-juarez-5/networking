/*
    Yonatan Juarez
    EE450 Summer 2024
    How to run:
    g++ -std=c++11 -o lab2 lab2.cpp
    ./lab2
*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <limits>
#include <queue>
#include <algorithm>

const int INF = std::numeric_limits<int>::max();
const int ID = 9;
void loadGraph(std::string filename, std::vector<std::vector<int> >* table);
void printInitialTable(std::vector<std::vector<int> >* table);
void dijkstra_pq(const std::vector<std::vector<int> >*table, int startNode);
void dijkstra(const std::vector<std::vector<int> >*table, int startNode);
int main() {
    
    std::string filename = "graph.csv";
    // student ID: 9778 5289 75
    int firstDigit = 4;
    
    std::vector<std::vector<int> > *table = new std::vector<std::vector<int> >();
    loadGraph(filename, table);
    // for (const auto& row : *table) {
    //     std::cout << "   ";
    //     for (const auto& cell : row) {
    //         std::cout << cell << "   ";
    //     }
    //     std::cout << std::endl;
    // }
    
    printInitialTable(table);
    dijkstra(table, 0);

    delete table;
    return 0;
}
void loadGraph(std::string filename, std::vector<std::vector<int> >* table){

      // Open the file
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        exit(0);
    }
     // Read the single line from the file
    std::string line;
    std::getline(file, line);
    file.close();

    // Replace semicolons with spaces to unify row separators
    std::replace(line.begin(), line.end(), ' ', ';');
    // exit(0);

    // Stream to handle the line parsing
    std::stringstream ss(line);
    std::string row;

    // Process each row of table
    while (std::getline(ss, row, ';')) {
        std::vector<int> rowData;
        std::stringstream rowStream(row);
        std::string cell;
        // std::cout << row << std::endl;
        while (std::getline(rowStream, cell, ',')) {
            // std::cout << cell << std::endl;
            if (cell == "") {
                rowData.push_back(-1); // Assuming -1 for no direct connection
                // std::cout << " " << " ";
            } else {
                rowData.push_back(std::stoi(cell));
                // std::cout << cell << " ";
            }
        }
        if (rowData.size() < 6){
            rowData.push_back(-1);
        }
        // exit(0);
        // std::cout << std::endl;
        table->push_back(rowData);
    }


}
void printInitialTable(std::vector<std::vector<int> > *table){
       // Output the adjacency matrix
    std::cout << "Initial Distance Table:" << std::endl;
    for (const auto& row : *table) {
        std::cout << "   ";
        for (const auto& cell : row) {
            if (cell == -1) {
                std::cout << " " << "   "; // Using 'inf' for no direct connection
            } else {
                std::cout << cell << "   ";
            }
        }
        std::cout << std::endl;
    }
}

void dijkstra_pq(const std::vector<std::vector<int> >*table, int startNode){
    std::cout << "Dijkstra Result:" << std::endl;
    int n = table->size();
    std::vector<int> distances(n, INF);
    std::vector<bool> visited(n, false);
    std::vector<int> parent(n, -1);
    distances[startNode] = 0;
    
     // Print the shortest distances
    std::unordered_map<int, char> nodes = {{0,'A'}, {1,'B'}, {2,'C'}, {3,'D'}, {4,'E'}, {5,'F'}};
    // minheap pq
    std::priority_queue<std::pair<int, int> , std::vector<std::pair<int,int> >, std::greater<std::pair<int, int>>> pq;
    // std::queue<>
    pq.push({0, startNode});

    while (!pq.empty()) {

        int node = pq.top().second; 
        // std::cout <<pq.top().first << ":"<< node << std::endl;
        pq.pop();

        if (visited[node]) continue;
        visited[node] = true;
        // Update the distances to the neighboring nodes
        for (int neighbor = 0; neighbor < n; neighbor++) {
            int weight = (*table)[node][neighbor];
            
            if (weight != -1 && !visited[neighbor]) {
                int newDist = distances[node] + weight;
                
                if(newDist < distances[neighbor] ) {
                    if (distances[neighbor] != ID){
                        distances[neighbor] = newDist;
                        pq.push({newDist, neighbor});
                        parent[neighbor] = node; // Update parent

                    }
                    
                }
            }
        }
    }
    
    // std::cout << "Shortest distances from node " << nodes[startNode] << ":" << std::endl;
    // for (int i = 0; i < n; i++) {
    //     if (distances[i] == INF) {
    //         std::cout << "Node " << i << ": inf" << std::endl;
    //     } else {
    //         std::cout << nodes[i] << ": " << distances[i] << std::endl;
    //     }
    // }
    // sort nodes for spanning tree
    std::vector<int> sortedNodes(n);
    for (int i = 0; i < n; i++) {
        sortedNodes[i] = i;
    }
    std::sort(sortedNodes.begin(), sortedNodes.end(), [&](int a, int b) {
        return distances[a] < distances[b];
    });
    // distances
    // for (int i =0; i < n; i++){
    //     std::cout << nodes[i] << ":" <<distances[i] << ", ";
    // }
       // Print the spanning tree in order of least distance
    std::cout << "Spanning Tree: ";
    for (int i = 0; i < n; i++) {
        if (distances[sortedNodes[i]] != INF) {
            std::cout << nodes[sortedNodes[i]];
        }
    }
    // std::cout << std::endl;
    // std::cout << "\n(Destination, Previous Node, distance from " << nodes[startNode] << "):" << std::endl;
    std::cout << "\n(Destination, Previous node, Distance)" << std::endl;
    for (int i = 0; i < n; i++) {
        if (i != startNode && distances[i] != INF) {
            std::cout << "(" << nodes[i] << ", " << (parent[i] != -1 ? nodes[parent[i]] : '-') << ", " << distances[i] << ")" << std::endl;
        }
    }

}

void dijkstra(const std::vector<std::vector<int> > *table, int startNode) {
    int n = table->size();
    std::vector<int> distances(n, INF);
    std::vector<bool> visited(n, false);
    std::vector<int> parent(n, -1);
    std::vector<int> visitOrder; // To track the order of node visits
    std::unordered_map<int, char> nodes = {{0, 'A'}, {1, 'B'}, {2, 'C'}, {3, 'D'}, {4, 'E'}, {5, 'F'}};
    distances[startNode] = 0;

    for (int i = 0; i < n; ++i) {
        // Find the unvisited node with the smallest distance
        int u = -1;
        for (int j = 0; j < n; ++j) {
            if (!visited[j] && (u == -1 || distances[j] < distances[u])) {
                u = j;
            }
        }
        if (distances[u] == INF) {
            break; // All remaining nodes are unreachable from startNode
        }
        // Mark the chosen node as visited
        visited[u] = true;
        visitOrder.push_back(u);
        // Update distances to neighboring nodes
        for (int v = 0; v < n; ++v) {
            if ((*table)[u][v] != -1 && !visited[v]) {
                int newDist = distances[u] + (*table)[u][v];
                if (newDist < distances[v]) {
                    if (distances[v] != ID){
                        distances[v] = newDist;
                        parent[v] = u;
                    }
                }
            }
        }
    }

    // Print the spanning tree
    std::cout<< "Spanning tree: ";
    for (int i : visitOrder) {
        
        // if (i != startNode && distances[i] != INF) {
            std::cout << nodes[i] ;
        // }
    }
    std::cout << "\n(Destination, Previous Node, Distance)" << std::endl;
    for (int i = 0; i < n; i++) {
        if (i != startNode && distances[i] != INF) {
            std::cout << "(" << nodes[i] << ", " << (parent[i] != -1 ? nodes[parent[i]] : '-') << ", " << distances[i] << ")" << std::endl;
        }
    }
}