#include <iostream>
#include <string>
#include <sstream>
#include "program.h"
using namespace std;


byte* Program::bytecode() {
    /*  Returns pointer bo a copy of the bytecode.
     *  Each call produces new copy.
     *
     *  Calling code is responsible for dectruction of the allocated memory.
     */
    byte* tmp = new byte[bytes];
    for (int i = 0; i < bytes; ++i) { tmp[i] = program[i]; }
    return tmp;
}

string Program::assembler() {
    /*  Returns a string representing the program written in
     *  Tatanka assembly code.
     *
     *  Any unused bytes are written out as PASS instructions
     *  to preserve addresses.
     */
    ostringstream out;
    return out.str();
}


int Program::size() {
    /*  Returns program size in bytes.
     */
    return bytes;
}


void Program::expand(int n) {
    /*  Expands program's byte array with n bytes.
     *  Expanded bytes are filled with PASS instructions to reset the memory location to
     *  some safe value.
     */
    byte* tmp = new byte[bytes+n];
    for (int i = 0; i < bytes; ++i) { tmp[i] = program[i]; }
    bytes += n;
    for (int i = (bytes-n); i < bytes; ++i) { tmp[i] = PASS; }
    delete[] program;
    program = tmp;
}

void Program::ensurebytes(int bts) {
    /*  Ensure program has at least bts bytes after current address.
     */
    if (bytes-addr_no < bts) {
    }
}


Program& Program::setAddressPtr(int addr) {
    /*  Sets address pointer to given index.
     *  Supports negative indexes.
     */
    addr_no = (addr >= 0 ? addr : bytes+addr);
    addr_ptr = program + addr_no;
    return (*this);
}


Program& Program::istore(int regno, int i) {
    /*  Appends istore instruction to bytecode.
     *
     *  Parameters:
     *
     *  regno:int - register number
     *  i:int     - value to store
     */
    ensurebytes(1 + 2*sizeof(int));

    program[addr_no++] = ISTORE;
    addr_ptr++;
    ((int*)addr_ptr)[0] = regno;
    ((int*)addr_ptr)[1] = i;
    addr_no += 2 * sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::pass() {
    /*  Inserts pass instruction.
     */
    ensurebytes(1);

    program[addr_no++] = PASS;
    addr_ptr++;

    return (*this);
}

Program& Program::halt() {
    /*  Inserts halt instruction.
     */
    ensurebytes(1);

    program[addr_no++] = HALT;
    addr_ptr++;

    return (*this);
}
