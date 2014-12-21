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

    byte program[128];  // bytecode for the program

    program[0] = ISTORE;          // set bytecode for ISTORE
    ((int*)(program+1))[0] = 1;   // set first operand for ISTORE to int 1 (register 1)
    ((int*)(program+1))[1] = 8;   // set second operand for ISTORE to int 4 (number 4)
    program[9] = ISTORE;
    ((int*)(program+10))[0] = 2;  // set first operand for ISTORE to int 1 (register 1)
    ((int*)(program+10))[1] = 16;  // set second operand for ISTORE to int 4 (number 4)
    program[18] = PRINT;
    ((int*)(program+19))[0] = 1;  // print integer in 1. register
    program[23] = PRINT;
    ((int*)(program+24))[0] = 2;  // print integer in 2. register
    program[28] = IADD;           // add integer...
    ((int*)(program+29))[0] = 1;  // ... from register 1
    ((int*)(program+29))[1] = 2;  // ... to register 2
    ((int*)(program+29))[2] = 3;  // ... and store the value in register 3
    program[41] = PRINT;
    ((int*)(program+42))[0] = 3;  // print integer in register 3
    program[46] = IDIV;           // divide integer...
    ((int*)(program+47))[0] = 3;  // ... from register 2
    ((int*)(program+47))[1] = 1;  // ... by register 3
    ((int*)(program+47))[2] = 4;  // ... and store the value in register 4
    program[59] = PRINT;
    ((int*)(program+60))[0] = 4;  // print integer in register 3
    program[64] = BRANCH;         // branch to bytecode to skip next two prints
    ((int*)(program+65))[0] = 123;
    program[69] = PRINT;
    ((int*)(program+70))[0] = 4;  // print integer in register 3
    program[74] = PRINT;
    ((int*)(program+75))[0] = 4;  // print integer in register 3

    program[123] = BRANCH;        // another BRANCH, just to show off
    ((int*)(program+124))[0] = 90;

    program[90] = IDEC;
    ((int*)(program+91))[0] = 4;
    program[95] = IINC;
    ((int*)(program+96))[0] = 4;
    program[100] = IDEC;
    ((int*)(program+101))[0] = 4;
    program[105] = PRINT;
    ((int*)(program+106))[0] = 4;

    program[110] = HALT;         // finally, a HALT instruction


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


    /*
    CPU cpu = CPU(64);
    cpu.load(program);
    cpu.run();
    */

    // Or more concisely:
    //return_code = CPU(64).load(program).run();
    return_code = CPU(64).load(Program().bytecode()).run();


    return return_code;
}
