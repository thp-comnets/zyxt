#include "CPLEX.cpp"

using namespace std;

int main(int argc, char *argv[]) {
    /*
    1. : main output dir
    2. : selected points file name
    3. : equipment file name
    4. : time limit
    5. : is removal apply
    */
    CPLEX cplex(argv[1], argv[2], argv[3], stod(argv[4]), static_cast<bool>(stoi(argv[5])), 1);
    cplex.populateModel(0); //edge Distance Limit. 0 is for no limit
    // cplex.writeModel("lp");
    cplex.solve();
    return 0;
}