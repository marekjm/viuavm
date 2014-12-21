#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "../version.h"
#include "../cpu.h"
#include "../bytecode.h"
#include "../program.h"
using namespace std;

typedef char byte;

int main(int argc, char* argv[]) {
    cout << "tatanka VM, version " << VERSION << endl;

    int return_code = 0;

    Program sample(128);

    sample.setAddressPtr(-1);
    sample.halt();
    sample.setAddressPtr();

    sample.istore(1, 8)
        .istore(2, 16)
        .print(1)
        .print(2)
        .iadd(1, 2, 3)
        .print(3)       // 5
        .idiv(3, 1, 4)
        .branch(14)
        .print(4)
        .iinc(4)
        .iinc(4)        // 10
        .idec(4)
        .print(4)
        .branch(15)
        .branch(8)
        .branch(-1)
        ;

    cout << "total instructions:  " << sample.instructionCount() << endl;
    cout << "branch calculation..." << endl;
    sample.calculateBranches();
    cout << "total instructions:  " << sample.instructionCount() << endl;
    cout << "total bytecode size: " << sample.size() << endl;

    return_code = CPU(64).load(sample.bytecode()).bytes(sample.size()).run();

    return return_code;
}
