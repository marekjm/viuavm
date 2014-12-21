#include <iostream>
#include <vector>
#include "bytecode.h"
#include "cpu.h"
#include "types/object.h"
#include "types/integer.h"
#include "types/boolean.h"
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


Object* CPU::fetchRegister(int i, bool nullok) {
    if (i >= reg_count) {
        throw "register access index out of bounds";
    }
    Object* optr = registers[i];
    if (!nullok and optr == 0) {
        throw "read from null register";
    }
    return optr;
}

int CPU::run(int cycles) {
    if (!bytecode) {
        throw "null bytecode (maybe not loaded?)";
    }
    int return_code = 0;

    int addr = 0;
    bool halt = false;
    bool branched;

    while (true) {
        branched = false;
        cout << "CPU: bytecode at 0x" << hex << addr << dec << ": ";

        try {
            switch (bytecode[addr]) {
                case ISTORE:
                    cout << "ISTORE " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << endl;
                    registers[ ((int*)(bytecode+addr+1))[0] ] = new Integer(((int*)(bytecode+addr+1))[1]);
                    addr += 2 * sizeof(int);
                    break;
                case IADD:
                    cout << "IADD " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() +
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case ISUB:
                    cout << "ISUB " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() -
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case IMUL:
                    cout << "IMUL " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() *
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case IDIV:
                    cout << "IDIV " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() /
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case IINC:
                    cout << "IINC " << ((int*)(bytecode+addr+1))[0] << endl;
                    (static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value())++;
                    addr += sizeof(int);
                    break;
                case IDEC:
                    cout << "IDEC " << ((int*)(bytecode+addr+1))[0] << endl;
                    (static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value())--;
                    addr += sizeof(int);
                    break;
                case ILT:
                    cout << "ILT " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Boolean( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() <
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case PRINT:
                    cout << "PRINT " << ((int*)(bytecode+addr+1))[0] << endl;
                    cout << fetchRegister(*((int*)(bytecode+addr+1)))->str() << endl;
                    addr += sizeof(int);
                    break;
                case BRANCH:
                    cout << "BRANCH 0x" << hex << *(int*)(bytecode+addr+1) << dec << endl;
                    if ((*(int*)(bytecode+addr+1)) == addr) {
                        throw "aborting: BRANCH instruction pointing to itself";
                    }
                    addr = *(int*)(bytecode+addr+1);
                    branched = true;
                    break;
                case RET:
                    cout << "RET " << *(int*)(bytecode+addr+1) << endl;
                    if (fetchRegister(*((int*)(bytecode+addr+1)))->type() == "Integer") {
                        registers[0] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() );
                    } else {
                        throw ("invalid return value: must be Integer but was: " + fetchRegister(*((int*)(bytecode+addr+1)))->str());
                    }
                    addr += sizeof(int);
                    break;
                case HALT:
                    cout << "HALT" << endl;
                    halt = true;
                    break;
                case PASS:
                    cout << "PASS" << endl;
                    break;
                default:
                    ostringstream error;
                    error << "unrecognised instruction (bytecode value: " << (int)bytecode[addr] << ")";
                    throw error.str().c_str();
            }
        } catch (const char* &e) {
            return_code = 1;
            cout << "exception: " << e << endl;
            break;
        }

        if (!branched) ++addr;
        if (addr >= cycles and cycles) break;
        if (halt) break;
    }

    if (return_code == 0 and registers[0]) {
        // if return code if the default one and
        // return register is not unused
        // copy value of return register as return code
        return_code = static_cast<Integer*>(registers[0])->value();
    }

    return return_code;
}
