#ifndef VIUA_CPU_H
#define VIUA_CPU_H

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <algorithm>
#include <stdexcept>
#include "../bytecode/bytetypedef.h"
#include "../types/type.h"
#include "registerset.h"
#include "frame.h"
#include "tryframe.h"
#include "../include/module.h"


const unsigned DEFAULT_REGISTER_SIZE = 256;


class HaltException : public std::runtime_error {
    public:
        HaltException(): std::runtime_error("execution halted") {}
};


class CPU {
#ifdef AS_DEBUG_HEADER
    public:
#endif
    /*  Bytecode pointer is a pointer to program's code.
     *  Size and executable offset are metadata exported from bytecode dump.
     */
    byte* bytecode;
    uint16_t bytecode_size;
    uint16_t executable_offset;

    // Global register set
    RegisterSet* regset;
    // Currently used register set
    RegisterSet* uregset;

    // Temporary register
    Type* tmp;

    // Static registers
    std::map<std::string, RegisterSet*> static_registers;

    /*  Call stack.
     */
    std::vector<Frame*> frames;
    Frame* frame_new;

    /*  Block stack.
     */
    std::vector<TryFrame*> tryframes;
    TryFrame* try_frame_new;

    /*  Function and block names mapped to bytecode addresses.
     */
    std::map<std::string, unsigned> function_addresses;
    std::map<std::string, unsigned> block_addresses;

    /*  Slot for thrown objects (typically exceptions).
     *  Can be set by user code and the CPU.
     */
    Type* thrown;
    Type* caught;

    /*  Variables set after CPU executed bytecode.
     *  They describe exit conditions of the bytecode that just stopped running.
     */
    int return_code;                // always set
    std::string return_exception;   // set if CPU stopped because of an exception
    std::string return_message;     // message set by exception

    unsigned instruction_counter;
    byte* instruction_pointer;

    /*  This is the interface between programs compiled to VM bytecode and
     *  extension libraries written in C++.
     */
    std::map<std::string, ExternalFunction*> external_functions;

    /*  Methods to deal with registers.
     */
    void updaterefs(Type* before, Type* now);
    bool hasrefs(unsigned);
    Type* fetch(unsigned) const;
    void place(unsigned, Type*);
    void ensureStaticRegisters(std::string);

    /*  Methods dealing with stack and frame manipulation.
     */
    Frame* requestNewFrame(int arguments_size = 0, int registers_size = 0);
    TryFrame* requestNewTryFrame();
    void pushFrame();
    void dropFrame();

    /*  Methods implementing CPU instructions.
     */
    byte* izero(byte*);
    byte* istore(byte*);
    byte* iadd(byte*);
    byte* isub(byte*);
    byte* imul(byte*);
    byte* idiv(byte*);

    byte* ilt(byte*);
    byte* ilte(byte*);
    byte* igt(byte*);
    byte* igte(byte*);
    byte* ieq(byte*);

    byte* iinc(byte*);
    byte* idec(byte*);

    byte* fstore(byte*);
    byte* fadd(byte*);
    byte* fsub(byte*);
    byte* fmul(byte*);
    byte* fdiv(byte*);

    byte* flt(byte*);
    byte* flte(byte*);
    byte* fgt(byte*);
    byte* fgte(byte*);
    byte* feq(byte*);

    byte* bstore(byte*);

    byte* itof(byte*);
    byte* ftoi(byte*);
    byte* stoi(byte*);
    byte* stof(byte*);

    byte* strstore(byte*);

    byte* vec(byte*);
    byte* vinsert(byte*);
    byte* vpush(byte*);
    byte* vpop(byte*);
    byte* vat(byte*);
    byte* vlen(byte*);

    byte* boolean(byte*);
    byte* lognot(byte*);
    byte* logand(byte*);
    byte* logor(byte*);

    byte* move(byte*);
    byte* copy(byte*);
    byte* ref(byte*);
    byte* swap(byte*);
    byte* free(byte*);
    byte* empty(byte*);
    byte* isnull(byte*);

    byte* ress(byte*);
    byte* tmpri(byte*);
    byte* tmpro(byte*);

    byte* print(byte*);
    byte* echo(byte*);

    byte* clbind(byte*);
    byte* closure(byte*);
    byte* clframe(byte*);
    byte* clcall(byte*);

    byte* function(byte*);
    byte* fcall(byte*);

    byte* frame(byte*);
    byte* param(byte*);
    byte* paref(byte*);
    byte* arg(byte*);

    byte* call(byte*);
    byte* end(byte*);

    byte* jump(byte*);
    byte* branch(byte*);

    byte* tryframe(byte*);
    byte* vmcatch(byte*);
    byte* pull(byte*);
    byte* vmtry(byte*);
    byte* vmthrow(byte*);
    byte* leave(byte*);

    byte* eximport(byte*);
    byte* excall(byte*);

    public:
        // debug and error reporting flags
        bool debug, errors;

        std::vector<std::string> commandline_arguments;

        /*  Public API of the CPU provides basic actions:
         *
         *      * load bytecode,
         *      * set its size,
         *      * tell the CPU where to start execution,
         *      * kick the CPU so it starts running,
         */
        CPU& load(byte*);
        CPU& bytes(uint16_t);
        CPU& eoffset(uint16_t);

        CPU& mapfunction(const std::string&, unsigned);
        CPU& mapblock(const std::string&, unsigned);

        CPU& registerExternalFunction(const std::string&, ExternalFunction*);
        CPU& removeExternalFunction(std::string);

        byte* begin();
        inline byte* end() { return 0; }

        CPU& iframe(Frame* frm = 0, unsigned r = DEFAULT_REGISTER_SIZE);

        byte* dispatch(byte*);
        byte* tick();

        int run();
        inline unsigned counter() { return instruction_counter; }

        inline std::tuple<int, std::string, std::string> exitcondition() {
            return std::tuple<int, std::string, std::string>(return_code, return_exception, return_message);
        }
        inline std::vector<Frame*> trace() { return frames; }

        CPU():
            bytecode(0), bytecode_size(0), executable_offset(0),
            regset(0), uregset(0),
            tmp(0),
            static_registers({}),
            frame_new(0),
            try_frame_new(0),
            thrown(0), caught(0),
            return_code(0), return_exception(""), return_message(""),
            instruction_counter(0), instruction_pointer(0),
            debug(false), errors(false)
        {}

        ~CPU() {
            /*  Destructor frees memory at bytecode pointer so make sure you passed a copy of the bytecode to the constructor
             *  if you want to keep it around after the CPU is finished.
             */
            if (bytecode) { delete[] bytecode; }
            for (std::pair<std::string, RegisterSet*> sr : static_registers) {
                delete sr.second;

                // this causes valgrind to SCREAM with errors...
                static_registers.erase(sr.first);
            }
        }
};

#endif
