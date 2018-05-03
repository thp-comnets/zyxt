INC_DIR = /opt/ibm/ILOG/CPLEX_Studio128/cplex/include
LIB = /opt/ibm/ILOG/CPLEX_Studio128/cplex/lib/x86-64_linux/static_pic

all:
	g++ -O0 -c -m64 -O -fPIC -fno-strict-aliasing -fexceptions -std=c++11 -DNDEBUG -DIL_STD -I/usr/include -I$(INC_DIR) -I$(INC_DIR)/../../concert/include main.cpp -o main.o
	g++ -O0 -m64 -fPIC -fno-strict-aliasing  -I$(INC_DIR) -I$(INC_DIR)/../../concert/include -L$(LIB) -L$(LIB)/../../../../concert/lib/x86-64_linux/static_pic -o main main.o -lgvc -lcgraph -lcdt -lconcert -lilocplex -lcplex -ldl -lm -lpthread	

clean:

	rm -f *.o main_two_nodes
