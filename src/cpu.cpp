#include <iostream>
#include <vector>
#include "bytecode.h"
#include "cpu.h"
using namespace std;


void CPU::load(vector<Instruction> ins) {
    instructions = ins;
}


int CPU::run(int cycles) {
    int return_code = 0;

    cout << "cpu: running" << endl;

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
        cout << "CPU::run(): instruction at " << hex << addr << ": ";

        // switch instructions
        switch ( inst.which ) {
            case BRANCH:
                cout << "BRANCH " << inst.operands << endl;
                branch = true;
                addr = inst.operands;
                break;
            case HALT:  // halt the CPU
                cout << "HALT" << endl;
                halt = true;
                break;
            case ISTORE:    // store an integer
                cout << "ISTORE " << inst.locals[0] << " " << inst.locals[1] << endl;
                registers[inst.locals[0]] = (void*)(new int(inst.locals[1]));
                break;
            case PRINT_I:
                cout << "PRINT_I " << inst.locals[0] << endl;
                cout << *((int*)registers[inst.locals[0]]) << endl;
                break;
            default:
                cout << endl;
        }

        // if address exceedes instruction vector size, we request the CPU to halt
        if (!branch) ++addr;
        if (addr >= end) halt = true;

        // if HALT was requested, we halt the CPU
        if (halt) break;
    }

    cout << "cpu: stopped" << endl;

    return return_code;
}
