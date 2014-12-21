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

int Program::instructionCount() {
    /*  Returns total number of instructions in the program.
     *  Should be called only after the program is constructed as it is calculated by
     *  bytecode analysis.
     */
    int counter = 0;
    for (int i = 0; i < bytes; ++i) {
        switch (program[i]) {
            case IADD:
            case ISUB:
            case IMUL:
            case IDIV:
                i += 3 * sizeof(int);
                break;
            case ISTORE:
                i += 2 * sizeof(int);
                break;
            case IINC:
            case IDEC:
            case PRINT:
            case BRANCH:
            case RET:
                i += sizeof(int);
                break;
        }
        ++counter;
    }
    return counter;
}


void Program::ensurebytes(int bts) {
    /*  Ensure program has at least bts bytes after current address.
     */
    if (bytes-addr_no < bts) {
        throw "not enough bytes";
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

int Program::getInstructionBytecodeOffset(int instr, int count) {
    /*  Returns bytecode offset for given instruction index.
     *  The "count" parameter is there to pass assumed instruction count to
     *  avoid recalculating it on every call.
     */

    // check if instruction count was passed, and calculate it if not
    count = (count >= 0 ? count : instructionCount());

    int offset = 0;
    for (int i = 0; i < (instr >= 0 ? instr : count+instr); ++i) {
        /*  The offset is automatically increased by one byte each time the loop is iterated over.
         *  This means that inside the switch statement, the offset has to be increased just by the
         *  number of bytes occupied by instruction operands.
         *
         *  It is done this way beacaue it is more ellegant than always writing '1 + <some offset>'
         *  every time, and defaulting to 'offset++'.
         */
        switch (program[offset++]) {
            case IADD:
            case ISUB:
            case IMUL:
            case IDIV:
                offset += 3 * sizeof(int);
                break;
            case ISTORE:
                offset += 2 * sizeof(int);
                break;
            case IINC:
            case IDEC:
            case PRINT:
            case BRANCH:
            case RET:
                offset += sizeof(int);
                break;
        }
        if (offset+1 > bytes) {
            throw "instruction offset out of bounds: check your branches";
        }
    }
    return offset;
}

Program& Program::calculateBranches() {
    /*  This function should be called after program is constructed
     *  to calculate correct bytecode offsets for BRANCH instructions.
     */
    int instruction_count = instructionCount();
    int* ptr;
    for (int i = 0; i < branches.size(); ++i) {
        ptr = (int*)(program+branches[i]+1);
        (*ptr) = getInstructionBytecodeOffset(*ptr, instruction_count);
    }
}


Program& Program::istore(int regno, int i) {
    /*  Inserts istore instruction to bytecode.
     *
     *  :params:
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

Program& Program::iadd(int rega, int regb, int regresult) {
    /*  Inserts iadd instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(int));

    program[addr_no++] = IADD;
    addr_ptr++;
    ((int*)addr_ptr)[0] = rega;
    ((int*)addr_ptr)[1] = regb;
    ((int*)addr_ptr)[2] = regresult;
    addr_no += 3 * sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::isub(int rega, int regb, int regresult) {
    /*  Inserts isub instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(int));

    program[addr_no++] = ISUB;
    addr_ptr++;
    ((int*)addr_ptr)[0] = rega;
    ((int*)addr_ptr)[1] = regb;
    ((int*)addr_ptr)[2] = regresult;
    addr_no += 3 * sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::imul(int rega, int regb, int regresult) {
    /*  Inserts imul instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(int));

    program[addr_no++] = IMUL;
    addr_ptr++;
    ((int*)addr_ptr)[0] = rega;
    ((int*)addr_ptr)[1] = regb;
    ((int*)addr_ptr)[2] = regresult;
    addr_no += 3 * sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::idiv(int rega, int regb, int regresult) {
    /*  Inserts idiv instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(int));

    program[addr_no++] = IDIV;
    addr_ptr++;
    ((int*)addr_ptr)[0] = rega;
    ((int*)addr_ptr)[1] = regb;
    ((int*)addr_ptr)[2] = regresult;
    addr_no += 3 * sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::iinc(int regno) {
    /*  Inserts idiv instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + sizeof(int));

    program[addr_no++] = IINC;
    addr_ptr++;
    ((int*)addr_ptr)[0] = regno;
    addr_no += sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::idec(int regno) {
    /*  Inserts idiv instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + sizeof(int));

    program[addr_no++] = IDEC;
    addr_ptr++;
    ((int*)addr_ptr)[0] = regno;
    addr_no += sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::branch(int addr) {
    /*  Inserts branch instruction. Parameter is instruction index.
     *  Byte offset is calculated automatically.
     *
     *  :params:
     *
     *  addr:int    - index of the instruction to which to branch
     */
    ensurebytes(1 + sizeof(int));

    // save branch instruction index for later evaluation
    branches.push_back(addr_no);

    program[addr_no++] = BRANCH;
    addr_ptr++;
    ((int*)addr_ptr)[0] = addr;
    addr_no += sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::print(int regno) {
    /*  Inserts print instuction.
     */
    ensurebytes(1 + sizeof(int));

    program[addr_no++] = PRINT;
    addr_ptr++;
    ((int*)addr_ptr)[0] = regno;
    addr_no += sizeof(int);
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
