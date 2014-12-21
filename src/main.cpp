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


    Program boolean_and_conditional_branching(192);

    boolean_and_conditional_branching.setAddressPtr(-1);
    boolean_and_conditional_branching.halt();
    boolean_and_conditional_branching.setAddressPtr();

    boolean_and_conditional_branching.istore(1, 4)
        .istore(2, 4)
        .print(1)
        .print(2)
        .ilt(1, 2, 3)
        .ilte(1, 2, 4)      // 5
        .igt(1, 2, 5)
        .igte(1, 2, 6)
        .ieq(1, 2, 7)
        .branchif(4, 15, 10)
        .print(3)           // 10
        .print(4)
        .print(5)
        .print(6)
        .print(7)
        .ret(2)             // 15
        .branch(-2)
        ;
    //boolean_and_conditional_branching.calculateBranches();


    // Sample program
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
    sample.calculateBranches();



    // display stats and run the program
    /*
    cout << "total instructions:  " << boolean_and_conditional_branching.instructionCount() << endl;
    cout << "total bytecode size: " << boolean_and_conditional_branching.size() << endl;

    CPU(64).load(boolean_and_conditional_branching.bytecode())
                         .bytes(boolean_and_conditional_branching.size())
                         //.run()
                         ;
                         */


    cout << "total instructions:  " << sample.instructionCount() << endl;
    cout << "total bytecode size: " << sample.size() << endl;

    CPU(64).load(sample.bytecode())
                         .bytes(sample.size())
                         .run()
                         ;

    return return_code;
}
