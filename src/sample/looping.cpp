#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include "../version.h"
#include "../cpu.h"
#include "../bytecode.h"
#include "../program.h"
using namespace std;

typedef char byte;

int main(int argc, char* argv[]) {
    cout << "tatanka VM, version " << VERSION << endl;

    int return_code = 0;

    Program sample(1024);    // 1KB

    sample.setAddressPtr(-1);
    sample.halt();
    sample.setAddressPtr();

    sample.istore(1, 0)         // store 0 in 1. register
          .istore(2, 10)        // store 10 in 2. register
          .ilt(1, 2, 3)         // compare: reg[3] = reg[1] < reg[2]
          .branchif(3, 4, 7)    // if (reg[3]) { branch 4; } else { branch 7; }
          .print(1)             // print register 1.
          .iinc(1)              // reg[1]++
          .branch(2)            // go back to ilt instruction
          .print(1)             // print value of 1. register
          .branch(-1)           // branch to the end of bytecode
          ;

    cout << "total instructions:  " << sample.instructionCount() << endl;
    cout << "calculating branches..." << endl;
    sample.calculateBranches();
    cout << "total instructions:  " << sample.instructionCount() << endl;
    cout << "total bytecode size: " << sample.size() << endl;

    ofstream out("looping.bin", ios::out | ios::binary);

    byte* bytecode = sample.bytecode();
    uint16_t bytecode_size = sample.size();

    out.write((const char*)&bytecode_size, 16);
    out.write(bytecode, sample.size());
    out.close();

    delete[] bytecode;

    CPU(64).load(sample.bytecode()).bytes(1024).run();

    return return_code;
}
