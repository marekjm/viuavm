#include <iostream>
#include <vector>
#include "bytecode.h"
#include "cpu.h"
#include "types/object.h"
#include "types/integer.h"
#include "types/boolean.h"
#include "types/byte.h"
#include "support/pointer.h"
using namespace std;


typedef char byte;


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


Object* CPU::fetchRegister(int i, bool nullok) {
    /*  Return pointer to object at given register.
     *  This method safeguards against reaching for out-of-bounds registers and
     *  reading from an empty register.
     *
     *  :params:
     *
     *  i:int       - index of a register to fetch
     *  nullok:bool - disables checking if reading from empty register
     */
    if (i >= reg_count) {
        throw "register access index out of bounds";
    }
    Object* optr = registers[i];
    if (!nullok and optr == 0) {
        throw "read from null register";
    }
    return optr;
}


char* CPU::istore(char* addr) {
    /*  Run istore instruction.
     */
    int reg, num;
    bool reg_ref = false, num_ref = false;

    reg_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    reg = *((int*)addr);
    pointer::inc<int, char>(addr);

    num_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "ISTORE";
        cout << (reg_ref ? " @" : " ") << reg;
        cout << (num_ref ? " @" : " ") << num;
        cout << endl;
    }

    if (reg_ref) {
        reg = static_cast<Integer*>(registers[reg])->value();
    }
    if (num_ref) {
        num = static_cast<Integer*>(registers[num])->value();
    }

    registers[reg] = new Integer(num);

    return addr;
}

char* CPU::ilt(char* addr) {
    /*  Run ilt instruction.
     */
    bool rega_ref, regb_ref, regr_ref;
    int rega_num, regb_num, regr_num;

    rega_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    rega_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regb_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regb_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    regr_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);
    regr_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "ILT";
        cout << (rega_ref ? " @" : " ") << rega_num;
        cout << (regb_ref ? " @" : " ") << regb_num;
        cout << (regr_ref ? " @" : " ") << regr_num;
        cout << endl;
    }

    if (rega_ref) {
        if (debug) { cout << "resolving reference to a-operand register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }
    if (regb_ref) {
        if (debug) { cout << "resolving reference to b-operand register" << endl; }
        regb_num = static_cast<Integer*>(registers[regb_num])->value();
    }
    if (regr_ref) {
        if (debug) { cout << "resolving reference to result register" << endl; }
        rega_num = static_cast<Integer*>(registers[rega_num])->value();
    }

    rega_num = static_cast<Integer*>(registers[rega_num])->value();
    regb_num = static_cast<Integer*>(registers[regb_num])->value();

    registers[regr_num] = new Boolean(rega_num < regb_num);

    return addr;
}

char* CPU::iinc(char* addr) {
    /*  Run iinc instruction.
     */
    bool ref = false;
    int regno;

    ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);

    regno = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "IINC" << (ref ? " @" : " ") << regno;
    }

    if (ref) {
        regno = static_cast<Integer*>(registers[regno])->value();
    }

    if (debug) {
        if (ref) { cout << " -> " << regno; }
        cout << endl;
    }

    ++(static_cast<Integer*>(registers[regno])->value());

    return addr;
}

char* CPU::echo(char* addr) {
    /*  Run echo instruction.
     */
    pointer::inc<bool, char>(addr);
    pointer::inc<int, char>(addr);
    return addr;
}

char* CPU::print(char* addr) {
    /*  Run print instruction.
     */
    bool ref = false;
    int reg;

    ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);

    reg = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "PRINT" << (ref ? " @" : " ") << reg;
        cout << endl;
    }

    if (ref) {
        reg = static_cast<Integer*>(registers[reg])->value();
    }

    cout << registers[reg]->str() << endl;

    return addr;
}

char* CPU::branch(char* addr) {
    /*  Run branch instruction.
     */
    if (debug) {
        cout << "BRANCH " << *(int*)addr << endl;
    }
    char* target = bytecode+(*(int*)addr);
    if (target == addr) {
        throw "aborting: BRANCH instruction pointing to itself";
    }
    return target;
}

char* CPU::branchif(char* addr) {
    /*  Run branchif instruction.
     */
    bool regcond_ref;
    int regcond_num;


    regcond_ref = *((bool*)addr);
    pointer::inc<bool, char>(addr);

    regcond_num = *((int*)addr);
    pointer::inc<int, char>(addr);

    int addr_true = *((int*)addr);
    pointer::inc<int, char>(addr);

    int addr_false = *((int*)addr);
    pointer::inc<int, char>(addr);

    if (debug) {
        cout << "BRANCHIF";
        cout << dec << (regcond_ref ? " @" : " ") << regcond_num;
        cout << " " << addr_true  << "::0x" << hex << (long)(bytecode+addr_true) << dec;
        cout << " " << addr_false << "::0x" << hex << (long)(bytecode+addr_false);
        cout << endl;
    }


    if (regcond_ref) {
        if (debug) { cout << "resolving reference to condition register" << endl; }
        regcond_num = static_cast<Integer*>(registers[regcond_num])->value();
    }

    bool result = registers[regcond_num]->boolean();

    addr = bytecode + (result ? addr_true : addr_false);

    return addr;
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
                    /*
                case IADD:
                    if (debug) cout << "IADD " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() +
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case ISUB:
                    if (debug) cout << "ISUB " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() -
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
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
                case BSTORE:
                    if (debug) cout << "BSTORE " << ((int*)(bytecode+addr+1))[0] << " " <<
                        (int)(((bytecode+addr+1+sizeof(int)))[0]) << endl;
                    registers[ ((int*)(bytecode+addr+1))[0] ] = new Byte(*(bytecode+addr+1+sizeof(int)));
                    addr += sizeof(int) + sizeof(char);
                    break;
                    */
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
                    break;
                default:
                    ostringstream error;
                    error << "unrecognised instruction (bytecode value: " << // (int)bytecode[addr] << ")";
                             *((int*)bytecode) << ")";
                    throw error.str().c_str();
            }
        } catch (const char* &e) {
            return_code = 1;
            cout << (debug ? "\n" : "") <<  "exception: " << e << endl;
            break;
        }

        if (halt) break;

        if (addr >= bytecode_size and bytecode_size) {
            cout << "CPU: aborting: bytecode address out of bound and no HALT instruction reached" << endl;
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
