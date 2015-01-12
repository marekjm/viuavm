#include <iostream>
#include <vector>
#include "../bytecode/opcode.h"
#include "../types/object.h"
#include "../types/integer.h"
#include "../types/boolean.h"
#include "../types/byte.h"
#include "../support/pointer.h"
#include "cpu.h"
using namespace std;


CPU& CPU::load(char* bc) {
    /*  Load bytecode into the CPU.
     *  CPU becomes owner of loaded bytecode - meaning it will consider itself responsible for proper
     *  destruction of it, so make sure you have a copy of the bytecode.
     *
     *  Any previously loaded bytecode is freed.
     *  To free bytecode without loading anything new it is possible to call .load(0).
     *
     *  :params:
     *
     *  bc:char*    - pointer to byte array containing bytecode with a program to run
     */
    if (bytecode) { delete[] bytecode; }
    bytecode = bc;
    return (*this);
}

CPU& CPU::bytes(uint16_t sz) {
    /*  Set bytecode size, so the CPU can stop execution even if it doesn't reach HALT instruction but reaches
     *  bytecode address out of bounds.
     */
    bytecode_size = sz;
    return (*this);
}

CPU& CPU::eoffset(uint16_t o) {
    /*  Set offset of first executable instruction.
     */
    executable_offset = o;
    return (*this);
}


Object* CPU::fetch(int index) {
    /*  Return pointer to object at given register.
     *  This method safeguards against reaching for out-of-bounds registers and
     *  reading from an empty register.
     *
     *  :params:
     *
     *  index:int   - index of a register to fetch
     */
    if (index >= reg_count) { throw "register access out of bounds: read"; }
    Object* optr = registers[index];
    if (optr == 0) { throw "read from null register"; }
    return optr;
}

void CPU::place(int index, Object* obj) {
    /** Place an object in register with given index.
     *
     * Before placing an object in register, a check is preformed if the register is empty.
     * If not - the `Object` previously stored in it is destroyed.
     */
    if (index >= reg_count) { throw "register access out of bounds: write"; }
    if (registers[index] != 0) {
        // register is not empty - the object in it must be destroyed to avoid memory leaks
        delete registers[index];
    }
    registers[index] = obj;
}


int CPU::run() {
    /*  VM CPU implementation.
     *
     *  A giant switch-in-while which iterates over bytecode and executes encoded instructions.
     */
    if (!bytecode) {
        throw "null bytecode (maybe not loaded?)";
    }
    int return_code = 0;

    bool halt = false;

    byte* instr_ptr = bytecode+executable_offset; // instruction pointer

    while (true) {
        if (debug) {
            cout << "CPU: bytecode ";
            cout << dec << ((long)instr_ptr - (long)bytecode);
            cout << " at 0x" << hex << (long)instr_ptr;
            cout << dec << ": ";
        }

        try {
            switch (*instr_ptr) {
                case ISTORE:
                    instr_ptr = istore(instr_ptr+1);
                    break;
                case IADD:
                    instr_ptr = iadd(instr_ptr+1);
                    break;
                case ISUB:
                    instr_ptr = isub(instr_ptr+1);
                    break;
                case IMUL:
                    instr_ptr = imul(instr_ptr+1);
                    break;
                case IDIV:
                    instr_ptr = idiv(instr_ptr+1);
                    break;
                case IINC:
                    instr_ptr = iinc(instr_ptr+1);
                    break;
                case IDEC:
                    instr_ptr = idec(instr_ptr+1);
                    break;
                case ILT:
                    instr_ptr = ilt(instr_ptr+1);
                    break;
                case ILTE:
                    instr_ptr = ilte(instr_ptr+1);
                    break;
                case IGT:
                    instr_ptr = igt(instr_ptr+1);
                    break;
                case IGTE:
                    instr_ptr = igte(instr_ptr+1);
                    break;
                case IEQ:
                    instr_ptr = ieq(instr_ptr+1);
                    break;
                case BSTORE:
                    instr_ptr = bstore(instr_ptr+1);
                    break;
                case NOT:
                    instr_ptr = lognot(instr_ptr+1);
                    break;
                case AND:
                    instr_ptr = logand(instr_ptr+1);
                    break;
                case OR:
                    instr_ptr = logor(instr_ptr+1);
                    break;
                case PRINT:
                    instr_ptr = print(instr_ptr+1);
                    break;
                case ECHO:
                    instr_ptr = echo(instr_ptr+1);
                    break;
                case JUMP:
                    instr_ptr = jump(instr_ptr+1);
                    break;
                case BRANCH:
                    instr_ptr = branch(instr_ptr+1);
                    break;
                    /*
                case RET:
                    if (debug) cout << "RET " << *(int*)(bytecode+addr+1) << endl;
                    if (fetchRegister(*((int*)(bytecode+addr+1)))->type() == "Integer") {
                        registers[0] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() );
                    } else {
                        throw ("invalid return value: must be Integer but was: " + fetchRegister(*((int*)(bytecode+addr+1)))->str());
                    }
                    addr += sizeof(int);
                    break;
                    */
                case HALT:
                    if (debug) cout << "HALT" << endl;
                    halt = true;
                    break;
                case PASS:
                    if (debug) cout << "PASS" << endl;
                    ++instr_ptr;
                    break;
                default:
                    ostringstream error;
                    error << "unrecognised instruction (bytecode value: " << *((int*)bytecode) << ")";
                    throw error.str().c_str();
            }
        } catch (const char* &e) {
            return_code = 1;
            cout << (debug ? "\n" : "") <<  "exception: " << e << endl;
            break;
        }

        if (halt) break;

        if (instr_ptr >= (bytecode+bytecode_size)) {
            cout << "CPU: aborting: bytecode address out of bounds" << endl;
            return_code = 1;
            break;
        }
    }

    if (return_code == 0 and registers[0]) {
        // if return code if the default one and
        // return register is not unused
        // copy value of return register as return code
        return_code = static_cast<Integer*>(registers[0])->value();
    }

    return return_code;
}
