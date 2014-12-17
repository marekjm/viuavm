#include <iostream>
#include <vector>
#include "src/bytecode.h"
using namespace std;


const int BYTES_SIZE = 64;
const int REGISTERS_SIZE = 64;


char bytes[BYTES_SIZE];
void* registers[REGISTERS_SIZE];


int cpu(char* bytecode) {
    int addr = 0;
    bool halt = false;

    while (true) {
        cout << "CPU: bytecode at 0x" << hex << addr << dec << "): ";

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

        if (++addr >= BYTES_SIZE) break;
        if (halt) break;
    }
}


int main() {
    bytes[0] = ISTORE;          // set bytecode for ISTORE
    ((int*)(bytes+1))[0] = 1;   // set first operand for ISTORE to int 1 (register 1)
    ((int*)(bytes+1))[1] = 1;   // set second operand for ISTORE to int 4 (number 4)
    bytes[9] = ISTORE;
    ((int*)(bytes+10))[0] = 2;  // set first operand for ISTORE to int 1 (register 1)
    ((int*)(bytes+10))[1] = 3;  // set second operand for ISTORE to int 4 (number 4)
    bytes[18] = PRINT_I;
    ((int*)(bytes+19))[0] = 1;  // print integer in 1. register
    bytes[23] = PRINT_I;
    ((int*)(bytes+24))[0] = 2;  // print integer in 2. register
    bytes[28] = IADD;           // add integer...
    ((int*)(bytes+29))[0] = 1;  // ... from register 1
    ((int*)(bytes+29))[1] = 2;  // ... to register 2
    ((int*)(bytes+29))[2] = 3;  // ... and store the value in register 3
    bytes[41] = PRINT_I;
    ((int*)(bytes+42))[0] = 3;  // print integer in register 3
    bytes[46] = HALT;

    cout << "CPU: initializing registers" << endl;
    for (int i = 0; i < REGISTERS_SIZE; ++i) registers[i] = 0;

    cout << "CPU: running (with " << REGISTERS_SIZE << " available registers)\n" << endl;

    cpu(bytes);

    cout << "\nCPU: stopped" << endl;
    cout << "CPU: freeing used registers" << endl;
    for (int i = 0; i < REGISTERS_SIZE; ++i) {
        if (registers[i]) {
            //cout << "CPU: freeing register " << i << endl;
            delete registers[i];
        }
    }

    return 0;
}
