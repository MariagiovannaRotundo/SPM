#include <iostream> 
#include <atomic>
#include <queue>
#include <chrono>
#include <fstream>
#include <filesystem>
#include "delay.cpp"

using namespace std;


typedef struct _nodes{
    int id;
    int value;
    int seen;
    int edges;
    int * nodes;
} nodes;

//number of nodes of the graph
int n;

//function to create the graph by a file
nodes * readGraph(string graph, int S){
    nodes * G;
    int i, j;
    
    fstream graphFile (graph, fstream::in);

    graphFile >> n;

    if(S<0 || S>=n){
        cout<<"invalid S"<<endl;
        exit(0);
    }
    
    G = (nodes *) malloc (n * sizeof(nodes));
    for(i=0; i<n; i++){
        G[i].id = i;
        G[i].seen = 0;
        graphFile >> G[i].value;
        graphFile >> G[i].edges;

        if(G[i].edges != 0){
            G[i].nodes =(int *) malloc((G[i].edges)*sizeof(int));
            for(j=0; j<G[i].edges; j++){
                graphFile >> G[i].nodes[j];
            }
        }
    }

    // Close the file
    graphFile.close();

    return G;
}

//function to count the occurrences of X starting from S
int occurrencies(nodes * G, int S, int X){

    int occ = 0;
    //queue of ids
    queue<int> q;

    G[S].seen = 1;
    q.push(G[S].id);

    while(q.size()){
        //get the next element to get and remove from the queue
        int elem = q.front();
        q.pop();
       
        //check S is an occurrency of X
        if(G[elem].value == X){
            occ++;
        }

        //delay(std::chrono::microseconds(20));

        //calculate what are the new reachable nodes
        for(int i=0; i<G[elem].edges; i++){
            if(!G[G[elem].nodes[i]].seen){
                G[G[elem].nodes[i]].seen = 1;
                q.push(G[elem].nodes[i]);
            } 
        }
    }

    return occ;
}



int main(int argc, char * argv[]) {

    if(argc != 4 ){
        //S is the id of the node, X the value.
        cout<<"./sequential S X graph"<<endl;
        return 0;
    }

    if(!filesystem::exists(argv[3])){
        cout<<"file with graph do not exists"<<endl;
        return 0;
    }

    //read the graph
    nodes * G = readGraph(argv[3], atoi(argv[1]));

    //read time and count the occurrences
    auto start = std::chrono::steady_clock::now();
    int occ = occurrencies(G, atoi(argv[1]), atoi(argv[2]));
    auto end = std::chrono::steady_clock::now();

    cout<<"occurrencies : "<<occ<<endl;
    cout<<"time (us): "<<std::chrono::duration_cast<std::chrono::microseconds>(end-start).count()<<endl;
    
    
    for(int i=0; i<n; i++){
        if(G[i].edges != 0){
            free(G[i].nodes);
        }
    }
    free(G);
}