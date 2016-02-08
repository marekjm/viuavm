#ifndef VIUA_PROGRAM_H
#define VIUA_PROGRAM_H

#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <viua/bytecode/bytetypedef.h>
#include <viua/cg/bytecode/instructions.h>


enum JUMPTYPE {
    JMP_RELATIVE = 0,
    JMP_ABSOLUTE,
    JMP_TO_BYTE,
};


class Program {
    // byte array containing bytecode
    byte* program;
    // size of the bytecode
    uint64_t bytes;

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

    uint64_t getInstructionBytecodeOffset(uint64_t, uint64_t count = 0);

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
    Program& opptr      (int_op, int_op);
    Program& swap       (int_op, int_op);
    Program& ress       (std::string);
    Program& tmpri      (int_op);
    Program& tmpro      (int_op);
    Program& opdelete   (int_op);
    Program& empty      (int_op);
    Program& isnull     (int_op, int_op);

    Program& print      (int_op);
    Program& echo       (int_op);

    Program& clbind     (int_op);
    Program& closure    (int_op, const std::string&);

    Program& function   (int_op, const std::string&);
    Program& fcall      (int_op, int_op);

    Program& frame      (int_op, int_op);
    Program& param      (int_op, int_op);
    Program& oppamv     (int_op, int_op);
    Program& paref      (int_op, int_op);
    Program& arg        (int_op, int_op);
    Program& argc       (int_op);

    Program& call       (int_op, const std::string&);
    Program& opthread   (int_op, const std::string&);
    Program& opthjoin   (int_op, int_op);
    Program& opthreceive(int_op);
    Program& opsupervisor(const std::string&);
    Program& jump       (uint64_t, enum JUMPTYPE);
    Program& branch     (int_op, uint64_t, enum JUMPTYPE, uint64_t, enum JUMPTYPE);

    Program& vmtry      ();
    Program& vmcatch    (std::string, std::string);
    Program& pull       (int_op);
    Program& vmenter    (std::string);
    Program& vmthrow    (int_op);
    Program& leave      ();

    Program& import     (std::string);
    Program& link       (std::string);

    Program& vmclass    (int_op, const std::string&);
    Program& vmderive   (int_op, const std::string&);
    Program& vmattach   (int_op, const std::string&, const std::string&);
    Program& vmregister (int_op);
    Program& vmnew      (int_op, const std::string&);
    Program& vmmsg      (int_op, const std::string&);

    Program& opreturn   ();
    Program& halt       ();


    /** Functions driving after-insertion calculations.
     *  These must be called after the bytecode is already generated as they must know
     *  size of the program.
     */
    Program& calculateBranches(unsigned offset = 0); // FIXME: is unused, scheduled for removal
    Program& calculateJumps(std::vector<std::tuple<uint64_t, uint64_t> >);
    std::vector<uint64_t> jumps();
    std::vector<uint64_t> jumpsAbsolute();

    byte* bytecode();
    Program& fill(byte*);

    Program& setdebug(bool d = true);
    Program& setscream(bool d = true);

    uint64_t size();
    int instructionCount();

    static uint64_t countBytes(const std::vector<std::string>&);

    Program(uint64_t bts = 2): bytes(bts), debug(false), scream(false) {
        program = new byte[bytes];
        /* Filling bytecode with zeroes (which are interpreted by CPU as NOP instructions) is a safe way
         * to prevent many hiccups.
         */
        for (decltype(bytes) i = 0; i < bytes; ++i) { program[i] = byte(0); }
        addr_ptr = program;
    }
    Program(const Program& that): program(nullptr), bytes(that.bytes), addr_ptr(nullptr), branches({}) {
        program = new byte[bytes];
        for (decltype(bytes) i = 0; i < bytes; ++i) {
            program[i] = that.program[i];
        }
        addr_ptr = program+(that.addr_ptr - that.program);
        for (unsigned i = 0; i < that.branches.size(); ++i) {
            branches.push_back(program+(that.branches[i]-that.program));
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
            for (decltype(bytes) i = 0; i < bytes; ++i) {
                program[i] = that.program[i];
            }
            addr_ptr = program+(that.addr_ptr - that.program);
            while (branches.size()) { branches.pop_back(); }
            for (unsigned i = 0; i < that.branches.size(); ++i) {
                branches.push_back(program+(that.branches[i]-that.program));
            }
        }
        return (*this);
    }
};


#endif
