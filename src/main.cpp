#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "version.h"
#include "cpu.h"
#include "bytecode.h"
#include "program.h"
using namespace std;

typedef char byte;

int main(int argc, char* argv[]) {
    cout << "tatanka VM, version " << VERSION << endl;

    int return_code = 0;


    byte boolean_and_branching_test[128];
    boolean_and_branching_test[0] = ISTORE;
    ((int*)(boolean_and_branching_test+1))[0] = 1;
    ((int*)(boolean_and_branching_test+1))[1] = 2;
    boolean_and_branching_test[9] = ISTORE;
    ((int*)(boolean_and_branching_test+10))[0] = 2;
    ((int*)(boolean_and_branching_test+10))[1] = 4;
    boolean_and_branching_test[18] = PRINT;
    ((int*)(boolean_and_branching_test+19))[0] = 1;
    boolean_and_branching_test[23] = PRINT;
    ((int*)(boolean_and_branching_test+24))[0] = 2;
    boolean_and_branching_test[28] = ILT;
    ((int*)(boolean_and_branching_test+29))[0] = 1;
    ((int*)(boolean_and_branching_test+29))[1] = 2;
    ((int*)(boolean_and_branching_test+29))[2] = 3;

    boolean_and_branching_test[41] = PRINT;
    ((int*)(boolean_and_branching_test+42))[0] = 3;

    boolean_and_branching_test[46] = RET;
    ((int*)(boolean_and_branching_test+47))[0] = 2;

    boolean_and_branching_test[51] = HALT;


    // Or more concisely:
    Program prog(128);

    prog.setAddressPtr(-1);
    prog.halt();
    prog.setAddressPtr();

    prog.istore(1, 8)
        .istore(2, 16)
        .print(1)
        .print(2)
        .iadd(1, 2, 3)
        .print(3)       // 5
        .idiv(3, 1, 4)
        .branch(13)
        .print(4)
        .iinc(4)
        .idec(4)        // 10
        .print(4)
        .branch(14)
        .branch(8)
        .branch(-1)
        ;

    prog.calculateBranches();

    cout << "total instructions:  " << prog.instructionCount() << endl;
    cout << "total bytecode size: " << prog.size() << endl;

    return_code = CPU(64).load(prog.bytecode()).run();


    return return_code;
}
