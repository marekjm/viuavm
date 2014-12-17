#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "version.h"
#include "cpu.h"
#include "bytecode.h"
using namespace std;


int main(int argc, char* argv[]) {
    cout << "tatanka VM, version " << VERSION << endl;

    vector<Instruction> program_halt;
    program_halt.push_back(Instruction(BRANCH, 3));  // 2
    program_halt.push_back(Instruction(HALT));       // 7
    program_halt.push_back(Instruction(BRANCH, 1));  // 3
    program_halt.push_back(Instruction(BRANCH, 2));  // 5
    program_halt.push_back(Instruction(HALT));       // 7


    vector<Instruction> program_iadd;
    Instruction istore = Instruction(ISTORE, 2);
    /*
    // ISTORE 1 4   # store number 4 in first register
    istore.local(0, 1);
    istore.local(1, 4);
     */
    istore.local(0, 1);
    istore.local(1, 4);
    program_iadd.push_back(istore);
    Instruction print_i = Instruction(PRINT_I, 1);
    print_i.local(0, 1);
    program_iadd.push_back(print_i);
    program_iadd.push_back(Instruction(HALT));

    CPU cpu = CPU();

    cpu.load(program_halt);
    cpu.run();

    return 0;
}
