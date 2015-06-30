#ifndef VIUA_PROGRAM_H
#define VIUA_PROGRAM_H

#include <string>
#include <vector>
#include <tuple>
#include <map>
#include "bytecode/bytetypedef.h"
#include "cg/bytecode/instructions.h"


class Program {
    // byte array containing bytecode
    byte* program;
    // size of the bytecode
    int bytes;

    /** Current address inside bytecode array.
     *  Used during bytecode generation.
     *  Methods implementing generation of specific bytecodes always
     *  append bytes to this pointer.
     */
    byte* addr_ptr;

    /** Branches inside bytecode must be stored for later recalculation.
     *  Absolute and local branches must be distinguished between as
     *  they are calculated a bit differently.
     */
    std::vector<byte*> branches;
    std::vector<byte*> branches_absolute;

    // simple, whether to print debugging information or not
    bool debug;
    bool scream;

    int getInstructionBytecodeOffset(int, int count = -1);

    public:
    // instruction insertion interface
    Program& nop        ();

    Program& izero      (int_op);
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

    Program& fstore     (int_op, float);
    Program& fadd       (int_op, int_op, int_op);
    Program& fsub       (int_op, int_op, int_op);
    Program& fmul       (int_op, int_op, int_op);
    Program& fdiv       (int_op, int_op, int_op);

    Program& flt        (int_op, int_op, int_op);
    Program& flte       (int_op, int_op, int_op);
    Program& fgt        (int_op, int_op, int_op);
    Program& fgte       (int_op, int_op, int_op);
    Program& feq        (int_op, int_op, int_op);

    Program& bstore     (int_op, byte_op);

    Program& itof       (int_op, int_op);
    Program& ftoi       (int_op, int_op);
    Program& stoi       (int_op, int_op);
    Program& stof       (int_op, int_op);

    Program& strstore   (int_op, std::string);

    Program& vec        (int_op);
    Program& vinsert    (int_op, int_op, int_op);
    Program& vpush      (int_op, int_op);
    Program& vpop       (int_op, int_op, int_op);
    Program& vat        (int_op, int_op, int_op);
    Program& vlen       (int_op, int_op);

    Program& lognot     (int_op);
    Program& logand     (int_op, int_op, int_op);
    Program& logor      (int_op, int_op, int_op);

    Program& move       (int_op, int_op);
    Program& copy       (int_op, int_op);
    Program& ref        (int_op, int_op);
    Program& swap       (int_op, int_op);
    Program& ress       (std::string);
    Program& tmpri      (int_op);
    Program& tmpro      (int_op);
    Program& free       (int_op);
    Program& empty      (int_op);
    Program& isnull     (int_op, int_op);

    Program& print      (int_op);
    Program& echo       (int_op);

    Program& clbind     (int_op);
    Program& closure    (std::string, int_op);

    Program& function   (std::string, int_op);
    Program& fcall      (int_op, int_op);

    Program& frame      (int_op, int_op);
    Program& param      (int_op, int_op);
    Program& paref      (int_op, int_op);
    Program& arg        (int_op, int_op);

    Program& call       (std::string, int_op);
    Program& jump       (int, enum JUMPTYPE);
    Program& branch     (int_op, int, enum JUMPTYPE, int, enum JUMPTYPE);

    Program& tryframe   ();
    Program& vmcatch    (std::string, std::string);
    Program& pull       (int_op);
    Program& vmtry      (std::string);
    Program& vmthrow    (int_op);
    Program& leave      ();

    Program& eximport   (std::string);
    Program& excall     (std::string, int_op);

    Program& end        ();
    Program& halt       ();


    /** Functions driving after-insertion calculations.
     *  These must be called after the bytecode is already generated as they must know
     *  size of the program.
     */
    Program& calculateBranches(unsigned offset = 0); // FIXME: is unused, scheduled for removal
    Program& calculateJumps(std::vector<std::tuple<int, int> >);
    std::vector<unsigned> jumps();
    std::vector<unsigned> jumpsAbsolute();

    byte* bytecode();
    Program& fill(byte*);

    Program& setdebug(bool d = true);
    Program& setscream(bool d = true);

    int size();
    int instructionCount();

    static uint16_t countBytes(const std::vector<std::string>&);

    Program(int bts = 2): bytes(bts), debug(false), scream(false) {
        program = new byte[bytes];
        /* Filling bytecode with zeroes (which are interpreted by CPU as NOP instructions) is a safe way
         * to prevent many hiccups.
         */
        for (int i = 0; i < bytes; ++i) { program[i] = byte(0); }
        addr_ptr = program;
    }
    Program(const Program& that): program(0), bytes(that.bytes), addr_ptr(0), branches({}) {
        program = new byte[bytes];
        for (int i = 0; i < bytes; ++i) {
            program[i] = that.program[i];
        }
        addr_ptr = program+long(that.addr_ptr - that.program);
        for (unsigned i = 0; i < that.branches.size(); ++i) {
            branches.push_back(program+long(that.branches[i]-that.program));
        }
    }
    ~Program() {
        delete[] program;
    }

    Program& operator=(const Program& that) {
        if (this != &that) {
            delete[] program;
            bytes = that.bytes;
            program = new byte[bytes];
            for (int i = 0; i < bytes; ++i) {
                program[i] = that.program[i];
            }
            addr_ptr = program+long(that.addr_ptr - that.program);
            while (branches.size()) { branches.pop_back(); }
            for (unsigned i = 0; i < that.branches.size(); ++i) {
                branches.push_back(program+long(that.branches[i]-that.program));
            }
        }
        return (*this);
    }
};


#endif
