INC_DIR = /home/cuda/Downloads/or-tools_Ubuntu-16.04-64bit_v6.7.4973/include
LIB = /home/cuda/Downloads/or-tools_Ubuntu-16.04-64bit_v6.7.4973/lib

INC_FCNF = /opt/ibm/ILOG/CPLEX_Studio128/cplex/include
LIB_FCNF = /opt/ibm/ILOG/CPLEX_Studio128/cplex/lib/x86-64_linux/static_pic

all:
	#MCF-RR
	g++ -fPIC -std=c++0x -O4 -DNDEBUG -I$(INC_DIR) -DARCH_K8 -Wno-deprecated -DUSE_CBC -DUSE_CLP -DUSE_GLOP  -c algorithms/MCF-RR/main.cpp -o algorithms/MCF-RR/mcf.o
	g++ -fPIC -std=c++0x -O4 -DNDEBUG -I$(INC_DIR) -DARCH_K8 -Wno-deprecated -DUSE_CBC -DUSE_CLP -DUSE_GLOP  algorithms/MCF-RR/mcf.o -Wl,-rpath $(LIB) -L$(LIB) -lortools -lz -lrt -lpthread -L/usr/lib -lgvc -lcgraph -lcdt -o algorithms/MCF-RR/mcf

	#FCNF
	g++ -O0 -c -m64 -O -fPIC -fno-strict-aliasing -fexceptions -std=c++11 -DNDEBUG -DIL_STD -I/usr/include -I$(INC_FCNF) -I$(INC_FCNF)/../../concert/include algorithms/FCNF/main.cpp -o algorithms/FCNF/main.o
	g++ -O0 -m64 -fPIC -fno-strict-aliasing  -I$(INC_FCNF) -I$(INC_FCNF)/../../concert/include -L$(LIB_FCNF) -L$(LIB_FCNF)/../../../../concert/lib/x86-64_linux/static_pic -o algorithms/FCNF/main algorithms/FCNF/main.o -lgvc -lcgraph -lcdt -lconcert -lilocplex -lcplex -ldl -lm -lpthread

	#Elevation Removal
	g++ -std=c++11 algorithms/elevationRemoval/elevationRemoval.cpp -o algorithms/elevationRemoval/elevationRemoval

	#Random Removal
	g++ -std=c++11 algorithms/randomRemoval/randomRemoval.cpp -o algorithms/randomRemoval/randomRemoval

	#Cluster Removal
	g++ -std=c++11 algorithms/clusteringRemoval/clusteringRemoval.cpp -o algorithms/clusteringRemoval/clusteringRemoval

	#Connectivity & Coverage Graph Generation
	g++ -std=c++11 algorithms/mapDiameterCalculation/mapDiameterCalculation.cpp -o algorithms/mapDiameterCalculation/mapDiameterCalculation

	#CUDA
	nvcc -gencode arch=compute_52,code=sm_52 -Xptxas=-v -o algorithms/CUDA/main algorithms/CUDA/main.cu

clean:
	rm -f algorithms/MCF-RR/*.o algorithms/MCF-RR/mcf
	rm -f algorithms/FCNF/*.o algorithms/FCNF/main
	rm -f algorithms/elevationRemoval/elevationRemoval
	rm -f algorithms/randomRemoval/randomRemoval
	rm -f algorithms/clusteringRemoval/clusteringRemoval
	rm -f algorithms/mapDiameterCalculation/mapDiameterCalculation
	rm -f algorithms/CUDA/main