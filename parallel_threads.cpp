#include <iostream> 
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <fstream>
#include <mutex> 
#include <condition_variable>
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


atomic<int> totalOcc;
mutex m1;  
condition_variable cv;

int numThread;
int barrier;
int level;

int n;

//function to create the graph by a file
nodes * readGraph(string graph, int S){
    nodes * G;
    int i, j;
    
    fstream graphFile (graph, fstream::in);

    graphFile >> n;
    
    if(S<0 || S>=n){
        std::cout<<"invalid S"<<std::endl;
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


//workers 
void worker(nodes * G, vector<int> &q1, vector<vector<int>> &q2, int X, int index){

    unique_lock<mutex> lk(m1, defer_lock);
    int occ = 0;
    int mylevel = 1;

    while(q1.size()){

        q2[index].clear();
        for(int i=index; i<q1.size(); i+=numThread){
            //get the next element to get and remove from the queue
            int elem = q1[i];
            
            //check S is an occurrency of X
            if(G[elem].value == X){
                occ++;
            }
            
            //delay(std::chrono::microseconds(20));

            //calculate what are the new reachable nodes
            if(G[elem].edges!=0){
                for(int i=0; i<G[elem].edges; i++){
                    if(!G[G[elem].nodes[i]].seen){
                        q2[index].push_back(G[elem].nodes[i]);
                    }
                }
            }
        }

        m1.lock();
        barrier++;
        
        //barrier: wait until all threads finish their computations
        if(barrier == numThread){//if the worker it's the last one
            q1.clear();
            //create the input vector for the next level to explore
            for(int j=0; j<numThread; j++){
                for(int i=0;i<q2[j].size();i++){
                    if(!G[q2[j][i]].seen){
                        G[q2[j][i]].seen = 1;
                        q1.push_back(q2[j][i]);
                    }
                }
            }
            barrier = 0;
            level++; 
            cv.notify_all();
        }
        else{
            while(barrier != 0 && mylevel == level){
                cv.wait(lk);
            }
        } 
        m1.unlock();
        mylevel++;
    }
    if(occ!=0)
        totalOcc +=occ;
}



int main(int argc, char * argv[]) {

    if(argc != 5 || atoi(argv[1])<0  || atoi(argv[3])<1){
        //S is the id of the node, X the value.
        cout<<"./sequential S X numThread graph"<<endl;
        return 0;
    }
    if(atoi(argv[3])<1){
        std::cout<<"invalid number of threads"<<std::endl;
        return 0;
    }

    if(!std::filesystem::exists(argv[4])){
        std::cout<<"file with graph do not exists"<<std::endl;
        return 0;
    }

    //read the graph
    nodes * G = readGraph(argv[4], atoi(argv[1]));
    //read the number of thread
    numThread = atoi(argv[3]);
    
    auto start = std::chrono::steady_clock::now();

    //queue of ids
    vector<int> q1;
    vector<vector<int>> q2;
    barrier = 0;
    level = 1;
    
    for (int i=0; i<numThread; i++){
        vector<int> qq2;
        q2.push_back(ref(qq2));
    }
    q1.push_back(G[atoi(argv[1])].id);

    //create threads worker
    thread workers[numThread];
    for (int i=0; i<numThread; i++){
        workers[i] = thread(worker, G, ref(q1), ref(q2), atoi(argv[2]), i);
    }
    //join
    for (int i=0; i<numThread; i++)
        workers[i].join();

    auto end = std::chrono::steady_clock::now();

    cout<<"occurrencies : "<<totalOcc<<endl;
    cout<<"time (us): "<<std::chrono::duration_cast<std::chrono::microseconds>(end-start).count()<<endl;
    
    for(int i=0; i<n; i++){
        if(G[i].edges != 0){
            free(G[i].nodes);
        }
    }
    free(G);
}