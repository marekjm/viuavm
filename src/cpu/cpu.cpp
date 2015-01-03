#include <iostream>
#include <vector>
#include "../bytecode.h"
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
    if (index >= reg_count) { throw "register access index out of bounds"; }
    Object* optr = registers[index];
    if (optr == 0) { throw "read from null register"; }
    return optr;
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

    int addr = 0;
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
                    /*
                case IMUL:
                    if (debug) cout << "IMUL " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() *
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case IDIV:
                    if (debug) cout << "IDIV " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() /
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                    */
                case IINC:
                    instr_ptr = iinc(instr_ptr+1);
                    break;
                    /*
                case IDEC:
                    if (debug) cout << "IDEC " << ((int*)(bytecode+addr+1))[0] << endl;
                    (static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value())--;
                    addr += sizeof(int);
                    break;
                    */
                case ILT:
                    instr_ptr = ilt(instr_ptr+1);
                    break;
                    /*
                case ILTE:
                    if (debug) cout << "ILTE " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Boolean( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() <=
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case IGT:
                    if (debug) cout << "IGT " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Boolean( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() >
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case IGTE:
                    if (debug) cout << "IGTE " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Boolean( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() >=
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case IEQ:
                    if (debug) cout << "IEQ " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Boolean( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() ==
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                    */
                case BSTORE:
                    instr_ptr = bstore(instr_ptr+1);
                    break;
                case PRINT:
                    instr_ptr = print(instr_ptr+1);
                    break;
                case ECHO:
                    instr_ptr = echo(instr_ptr+1);
                    break;
                case BRANCH:
                    instr_ptr = branch(instr_ptr+1);
                    break;
                case BRANCHIF:
                    instr_ptr = branchif(instr_ptr+1);
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
