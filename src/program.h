#ifndef WUDOO_PROGRAM_H
#define WUDOO_PROGRAM_H

#include <string>
#include <vector>
#include <tuple>
#include "bytecode/bytetypedef.h"

typedef std::tuple<bool, int> int_op;
typedef std::tuple<bool, byte> byte_op;

class Program {
    byte* program;
    int bytes;

    int addr_no;
    byte* addr_ptr;

    std::vector<int> branches;
    std::vector<int> ifbranches;

    bool debug;

    void ensurebytes(int);
    int getInstructionBytecodeOffset(int, int count = -1);

    public:
    // instructions interface
    Program& istore     (int_op, int_op);
    Program& iadd       (int_op, int_op, int_op);
    Program& isub       (int_op, int_op, int_op);
    Program& imul       (int_op, int_op, int_op);
    Program& idiv       (int_op, int_op, int_op);

    Program& iinc       (int_op);
    Program& idec       (int_op);

    Program& ilt        (int_op, int_op, int_op);
    Program& ilte       (int_op, int_op, int_op);
    Program& igt        (int_op, int_op, int_op);
    Program& igte       (int_op, int_op, int_op);
    Program& ieq        (int_op, int_op, int_op);

    Program& bstore     (int_op, byte_op);

    Program& lognot     (int_op);
    Program& logand     (int_op, int_op, int_op);
    Program& logor      (int_op, int_op, int_op);

    Program& move       (int_op, int_op);
    Program& copy       (int_op, int_op);

    Program& print      (int_op);
    Program& echo       (int_op);

    Program& jump       (int);
    Program& branch     (int_op, int, int);

    Program& ret        (int);
    Program& end        ();

    Program& pass       ();
    Program& halt       ();

    Program& calculateBranches();


    // representations
    byte* bytecode();

    Program& setdebug(bool d = true);

    int size();
    int instructionCount();


    Program(int bts = 2): bytes(bts), debug(false) {
        program = new byte[bytes];
        for (int i = 0; i < bytes; ++i) { program[i] = byte(0); }

        addr_no = 0;
        addr_ptr = program+addr_no;
    }
    ~Program() {
        delete[] program;
    }
};


#endif
