#include <iostream>
#include <vector>
#include "bytecode.h"
#include "cpu.h"
using namespace std;


/*
void* CPU::getRegister(int index) {
    if (index >= registers.size()) { throw "register index out of bounds"; }
    if (registers[index] == 0) { throw "read from empty register"; }
    return registers[index];
}


int CPU::run(int cycles) {
    int return_code = 0;

    cout << "CPU: running (with " << registers.size() << " active registers)" << endl;

    bool halt = false;  // whether to halt cpu or not yet
    int addr = 0;       // address of current instruction
    int end = instructions.size();  // if addr exceeeds size, it is time to stop
    bool branch;    // if a BRANCH was issued, do not increase the instruction address

    while (true) {
        // reset BRANCH flag
        branch = false;

        // fetch instruction under current address
        Instruction inst = instructions[addr];

        // print out where are we
        cout << "CPU: instruction at " << hex << addr << ": ";

        try {
            // switch instructions
            switch ( inst.which ) {
                case BRANCH:    // branch to an address
                    cout << "BRANCH " << inst.locals[0] << endl;
                    branch = true;
                    addr = inst.locals[0];
                    break;
                case HALT:      // halt the CPU
                    cout << "HALT" << endl;
                    halt = true;
                    break;
                case ISTORE:    // store an integer
                    cout << "ISTORE " << inst.locals[0] << " " << inst.locals[1] << endl;
                    registers[inst.locals[0]] = (void*)(new int(inst.locals[1]));
                    break;
                case IADD:      // add two integers
                    cout << "IADD " << inst.locals[0] << " " << inst.locals[1] << " " << inst.locals[2] << endl;
                    registers[inst.locals[2]] = (void*)(new int(*(int*)registers[inst.locals[0]] + *(int*)registers[inst.locals[1]]));
                    break;
                case PRINT_I:   // print an integer
                    cout << "PRINT_I " << inst.locals[0] << endl;
                    cout << *((int*)getRegister(inst.locals[0])) << endl;
                    break;
                default:
                    cout << endl;
            }
        } catch (const char* &e) {
            cout << endl << "exception at instruction " << hex << addr << ": " << e << endl;
            return_code = 1;
            break;
        }

        // if address exceedes instruction vector size, we request the CPU to halt
        if (!branch) ++addr;
        if (addr >= end) halt = true;

        // if HALT was requested, we halt the CPU
        if (halt) break;
    }

    cout << "CPU: stopped";
    if (return_code) cout << " (execution aborted)";
    cout << endl;

    return return_code;
}
*/


int CPU::run2(int cycles) {
    if (!bytecode) {
        throw "null bytecode (maybe not loaded?)";
    }
    int return_code = 0;

    int addr = 0;
    bool halt = false;
    bool branched;

    while (true) {
        cout << "CPU: bytecode at 0x" << hex << addr << dec << ": ";

        switch (bytecode[addr]) {
            case ISTORE:
                cout << "ISTORE " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << endl;
                registers[ ((int*)(bytecode+addr+1))[0] ] = (void*)(new int(((int*)(bytecode+addr+1))[1]));
                addr += 2 * sizeof(int);
                break;
            case IADD:
                cout << "IADD " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                registers[ ((int*)(bytecode+addr+1))[2] ] = (void*)(new int( *(int*)registers[((int*)(bytecode+addr+1))[0]] + *(int*)registers[((int*)(bytecode+addr+1))[1]] ));
                addr += 3 * sizeof(int);
                break;
            case PRINT_I:
                cout << "PRINT_I " << ((int*)(bytecode+addr+1))[0] << endl;
                cout << *(int*)registers[*((int*)(bytecode+addr+1))] << endl;// (void*)(new int(((int*)(bytecode+addr+1))[1]));
                addr += sizeof(int);
                break;
            case HALT:
                cout << "HALT" << endl;
                halt = true;
                break;
        }

        if (++addr >= cycles and cycles) break;
        if (halt) break;
    }

    return 0;
}
