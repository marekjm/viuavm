#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "version.h"
#include "cpu.h"
#include "bytecode.h"
using namespace std;

#define byte char


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
    ((int*)(program+124))[0] = 108;

    program[0x6c] = HALT;         // finally, a HALT


    /*
    CPU cpu = CPU(64);
    cpu.load(program);
    cpu.run();
    */

    // Or more concisely:
    CPU(64).load(program).run();


    return return_code;
}
