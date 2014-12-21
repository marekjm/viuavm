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
     */
    byte* tmp = new byte[bytes+n];
    for (int i = 0; i < bytes; ++i) { tmp[i] = program[i]; }
    bytes += n;
}


Program& istore(int regno, int i) {
}
