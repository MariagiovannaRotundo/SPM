#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <chrono>
#include <ff/ff.hpp>
#include <ff/farm.hpp>
#include <filesystem>
#include "delay.cpp"

using namespace ff;


typedef struct _nodes{
    int id;
    int value;
    int seen;
    int edges;
    int * nodes;
} nodes;

typedef struct _result{
    int occ;
    std::vector<int> output;
} result;

int n;
int numThread;


//function to create the graph by a file
nodes * readGraph(std::string graph, int S){
    nodes * G;
    int i, j;
    
    std::fstream graphFile (graph, std::fstream::in);

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
        } else {
            G[i].nodes = NULL;
        }
    }

    // Close the file
    graphFile.close();

    return G;
}



//Emitter
struct Emitter:ff_monode_t<result,std::vector<int>> {
    Emitter(nodes *G, const int S):G(G), S(S) {}

    //initialize variables
    int svc_init() {
        barrier_level = 0;
        occ=0;
        stop = true;
        j = 0;
        for (int i=0; i<numThread; i++){
            std::vector<int> ql1;
            q1.push_back(ref(ql1));
            std::vector<int> ql2;
            q2.push_back(ref(ql2));
        }

        return 0;
    }

    std::vector<int>* svc(result* in) {
        
        if(in!=nullptr){ //receives a result from the workers
            barrier_level++;
            occ += in->occ;
            
            //insert new nodes in the input for the next level to explore
            for(int i=0; i<(in->output).size(); i++){
                if(!G[(in->output)[i]].seen){
                    G[(in->output)[i]].seen = 1;
                    q2[j].push_back((in->output)[i]);
                    stop=false;
                    j = (j+1)%numThread;
                }
            }
            //all threads have finished the computation of the input
            if(barrier_level == numThread){
                if(stop){
                    return EOS;
                } else {
                    std::swap(q1, q2); 
                    
                    barrier_level = 0;
                    stop = true;
                    j = 0;
                    //start the exploration of the next level
                    for(int i=0;i<numThread;i++) {
                        q2[i].clear();
                        ff_send_out(&q1[i], i);
                    }
                    return GO_ON;
                }
            } else{
                return GO_ON;
            }
            
        } else { 
            //gives the root of the BFS to the first worker
            G[S].seen = 1;
            q1[0].push_back(G[S].id);
            for(int i=0;i<numThread;i++) {
                ff_send_out(&q1[i], i);
            }
            return GO_ON;
        }
        
    }

    nodes *G;
    const int S;

    std::vector<std::vector<int>> q1;
    std::vector<std::vector<int>> q2;

    int barrier_level;
    int occ;
    int j;
    bool stop;
};


//Workers
struct Worker:ff_node_t<std::vector<int>, result> {
    Worker(nodes * G, const int X): G(G), X(X){}

    //inizialization of variables
    int svc_init() {
        out = NULL;
        return 0;
    }

    result* svc(std::vector<int>* in) {
        
        out = new result();
        out->occ = 0;
        out->output.clear();

        for(int i=0; i<(*in).size(); i++){
            
            //get the next element to get and remove from the queue
            int elem = (*in)[i];
            //check S is an occurrency of X
            if(G[elem].value == X){
                out->occ++;
            }
            
            //delay(std::chrono::microseconds(20));

            //calculate what are the new reachable nodes
            if(G[elem].edges!=0){
                for(int i=0; i<G[elem].edges; i++){
                    if(!G[G[elem].nodes[i]].seen){
                        out->output.push_back(G[elem].nodes[i]);
                    } 
                }
            }
        }
        
        // it returns the partial occurrences and the found nodes
        return out; 
    }

    nodes *G;
    const int X;
    result * out;
};




int main(int argc, char * argv[]){

    if(argc != 5){
        //S is the id of the node, X the value.
        std::cout<<"./master_worker_no_lock S X numThread graph"<<std::endl;
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

    //read the graph and the number of threads
    nodes * G = readGraph(argv[4], atoi(argv[1]));
    numThread = atoi(argv[3]);


    ffTime(START_TIME);  
      
    std::vector<std::unique_ptr<ff_node>> W;
    for(int i=0; i<numThread; i++){
        W.push_back(make_unique<Worker>(G, atoi(argv[2])));
    }
    Emitter E(G, atoi(argv[1]));

    ff_Farm<> farm(std::move(W),E);	       
    farm.remove_collector();
    farm.wrap_around();
   
    if (farm.run_and_wait_end()<0) {
        error("running farm");
        return -1;
    }

    ffTime(STOP_TIME);
    
    std::cout << "occurrencies: " << E.occ << "\ntime (ms): "<< ffTime(GET_TIME) << std::endl;
    
    for(int i=0; i<n; i++){
        if(G[i].edges != 0){
            free(G[i].nodes);
        }
    }
    free(G);
    
    return(0);
}
