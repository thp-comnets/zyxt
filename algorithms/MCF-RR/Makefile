INC_DIR = or-tools_Ubuntu-16.04-64bit_v6.7.4973/include
LIB = or-tools_Ubuntu-16.04-64bit_v6.7.4973/lib

all:
	g++ -fPIC -std=c++0x -O4 -DNDEBUG -I$(INC_DIR) -DARCH_K8 -Wno-deprecated -DUSE_CBC -DUSE_CLP -DUSE_GLOP  -c main.cpp -o mcf.o
	g++ -fPIC -std=c++0x -O4 -DNDEBUG -I$(INC_DIR) -DARCH_K8 -Wno-deprecated -DUSE_CBC -DUSE_CLP -DUSE_GLOP  mcf.o -Wl,-rpath $(LIB) -L$(LIB) -lortools -lz -lrt -lpthread -L/usr/lib -lgvc -lcgraph -lcdt -o mcf

with_removal:
	g++ -fPIC -std=c++0x -O4 -DNDEBUG -I$(INC_DIR) -DARCH_K8 -Wno-deprecated -DUSE_CBC -DUSE_CLP -DUSE_GLOP  -c main_with_removal.cpp -o mcf_removal.o
	g++ -fPIC -std=c++0x -O4 -DNDEBUG -I$(INC_DIR) -DARCH_K8 -Wno-deprecated -DUSE_CBC -DUSE_CLP -DUSE_GLOP  mcf_removal.o -Wl,-rpath $(LIB) -L$(LIB) -lortools -lz -lrt -lpthread -L/usr/lib -lgvc -lcgraph -lcdt -o mcf_removal

clean:
	rm -f *.o mcf
	rm -f *.o mcf_removal
