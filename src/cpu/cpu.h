#ifndef TATANKA_CPU_H
#define TATANKA_CPU_H

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <algorithm>
#include "../bytecode/bytetypedef.h"
#include "../types/object.h"
#include "frame.h"


const int DEFAULT_REGISTER_SIZE = 256;


class CPU {
    /*  Bytecode pointer is a pointer to program's code.
     *  Size and executable offset are metadata exported from bytecode dump.
     */
    byte* bytecode;
    uint16_t bytecode_size;
    uint16_t executable_offset;

    /*  Registers and their number stored.
     */
    Object** registers;
    bool* references;
    int reg_count;

    // Currently used register set
    Object** uregisters;
    bool* ureferences;
    int uregisters_size;

    // Temporary register
    Object* tmp;

    // Static registers
    std::map<std::string, std::tuple<Object**, bool*, int> > static_registers;

    /*  Call stack.
     */
    std::vector<Frame*> frames;
    Frame* frame_new;

    /*  Function ID to bytecode address map.
     */
    std::map<std::string, unsigned> function_addresses;

    /*  Variables set after CPU executed bytecode.
     *  They describe exit conditions of the bytecode that just stopped running.
     */
    int return_code;                // always set
    std::string return_exception;   // set if CPU stopped because of an exception
    std::string return_message;     // message set by exception

    unsigned instruction_counter;

    /*  Methods to deal with registers.
     */
    void updaterefs(Object* before, Object* now);
    bool hasrefs(int index);
    Object* fetch(int);
    void place(int, Object*);
    void ensureStaticRegisters(std::string);

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

    byte* strstore(byte*);

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

    byte* frame(byte*);
    byte* param(byte*);
    byte* paref(byte*);
    byte* arg(byte*);

    byte* call(byte*);
    byte* end(byte*);

    byte* jump(byte*);
    byte* branch(byte*);

    public:
        // debug and error reporting flags
        bool debug, errors;
        // stepping flag, enables to run CPU in a step-by-step mode
        bool stepping;

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

        int run();
        inline unsigned counter() { return instruction_counter; }

        inline std::tuple<int, std::string, std::string> exitcondition() {
            return std::tuple<int, std::string, std::string>(return_code, return_exception, return_message);
        }
        inline std::vector<Frame*> trace() { return frames; }

        CPU(int r = DEFAULT_REGISTER_SIZE):
            bytecode(0), bytecode_size(0), executable_offset(0),
            registers(0), references(0), reg_count(r),
            uregisters(0), ureferences(0), uregisters_size(0),
            tmp(0),
            static_registers({}),
            frame_new(0),
            return_code(0), return_exception(""), return_message(""),
            instruction_counter(0),
            debug(false), errors(false), stepping(false)
        {
            /*  Basic constructor.
             *  Creates registers array of requested size and
             *  initializes it with zeroes.
             */
            registers = new Object*[reg_count];
            references = new bool[reg_count];
            for (int i = 0; i < reg_count; ++i) {
                registers[i] = 0;
                references[i] = false;
            }
            uregisters = registers;
            ureferences = references;
            uregisters_size = reg_count;

            if (stepping) { debug = true; }
        }

        ~CPU() {
            /*  Destructor frees memory at bytecode pointer so make sure you passed a copy of the bytecode to the constructor
             *  if you want to keep it around after the CPU is finished.
             */
            if (bytecode) { delete[] bytecode; }
            for (std::pair<std::string, std::tuple<Object**, bool*, int> > sr : static_registers) {
                Object** static_registers_to_free;
                bool* static_references_to_free;
                int static_registers_size_to_free;
                std::tie(static_registers_to_free, static_references_to_free, static_registers_size_to_free) = sr.second;
                std::cout << "freeing static registers of: '" << sr.first << "'" << std::endl;
                for (int i = 0; i < static_registers_size_to_free; ++i) {
                    if (static_registers_to_free[i]) {
                        delete static_registers_to_free[i];
                    }
                }
                delete[] static_references_to_free;
                delete[] static_registers_to_free;
                static_registers.erase(sr.first);
            }
        }
};

#endif
