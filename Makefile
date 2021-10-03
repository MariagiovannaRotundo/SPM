CXX		= g++ -std=c++17
CXXFLAGS  	= -g -O3
LDFLAGS 	= -pthread
INCLUDES	= -I ${HOME}/fastflow

TARGETS		= sequential parallel_threads master_worker


.PHONY: all clean cleanall
.SUFFIXES: .cpp 

%: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS)  $< -o $@  

all			: 
	$(CXX) $(CXXFLAGS) sequential.cpp -o sequential
	$(CXX) $(CXXFLAGS) $(LDFLAGS) parallel_threads.cpp  -o parallel_threads
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(INCLUDES)  master_worker.cpp -o master_worker

clean		: 
	rm -f $(TARGETS)

cleanall	: clean
	rm -f *.o *~

