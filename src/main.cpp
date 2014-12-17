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

    int return_code = 0;

    char program[128];  // bytecode for the program

    program[0] = ISTORE;          // set bytecode for ISTORE
    ((int*)(program+1))[0] = 1;   // set first operand for ISTORE to int 1 (register 1)
    ((int*)(program+1))[1] = 1;   // set second operand for ISTORE to int 4 (number 4)
    program[9] = ISTORE;
    ((int*)(program+10))[0] = 2;  // set first operand for ISTORE to int 1 (register 1)
    ((int*)(program+10))[1] = 3;  // set second operand for ISTORE to int 4 (number 4)
    program[18] = PRINT_I;
    ((int*)(program+19))[0] = 1;  // print integer in 1. register
    program[23] = PRINT_I;
    ((int*)(program+24))[0] = 2;  // print integer in 2. register
    program[28] = IADD;           // add integer...
    ((int*)(program+29))[0] = 1;  // ... from register 1
    ((int*)(program+29))[1] = 2;  // ... to register 2
    ((int*)(program+29))[2] = 3;  // ... and store the value in register 3
    program[41] = PRINT_I;
    ((int*)(program+42))[0] = 3;  // print integer in register 3
    program[46] = HALT;


    CPU cpu = CPU(64, program);

    cpu.run2();

    return return_code;
}
