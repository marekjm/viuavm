#ifndef TATANKA_PROGRAM_H
#define TATANKA_PROGRAM_H

#include <string>
#include <vector>
#include "bytecode.h"

typedef char byte;

class Program {
    byte* program;
    int bytes;

    int addr_no;
    byte* addr_ptr;

    std::vector<int> branches;
    std::vector<int> ifbranches;

    void ensurebytes(int);
    int getInstructionBytecodeOffset(int, int count = -1);

    public:
    // instructions interface
    Program& istore     (int, int);
    Program& iadd       (int, int, int);
    Program& isub       (int, int, int);
    Program& imul       (int, int, int);
    Program& idiv       (int, int, int);
    Program& iinc       (int);
    Program& idec       (int);
    Program& ilt        (int, int, int);
    Program& ilte       (int, int, int);
    Program& igt        (int, int, int);
    Program& igte       (int, int, int);
    Program& ieq        (int, int, int);

    Program& strstore   (int, std::string);

    Program& print      (int);

    Program& branch     (int);
    Program& branchif   (int, int, int);

    Program& ret        (int);
    Program& end        ();

    Program& pass       ();
    Program& halt       ();

    Program& setAddressPtr(int n = 0);
    Program& calculateBranches();

    // representations
    byte* bytecode();
    std::string assembler();


    int size();
    int instructionCount();


    Program(int bts = 2): bytes(bts) {
        program = new byte[bytes];
        for (int i = 0; i < bytes; ++i) { program[i] = PASS; }

        addr_no = 0;
        addr_ptr = program+addr_no;
    }
    ~Program() {
        delete[] program;
    }
};


#endif
