#include <iostream>
#include <string>
#include <sstream>
#include "program.h"
using namespace std;

typedef std::tuple<bool, int> int_op;
typedef std::tuple<bool, byte> byte_op;


template<class T, class S> void inc(S*& p) {
    /*  Increase pointer of type S, as if it were of type T.
     */
    T* ptr = (T*)p;
    ptr++;
    p = (S*)ptr;
}


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
            case ILT:
            case ILTE:
            case IGT:
            case IGTE:
            case IEQ:
            case BRANCHIF:
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
            case ILT:
            case ILTE:
            case IGT:
            case IGTE:
            case IEQ:
            case BRANCHIF:
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
    bool conditional = false;
    for (int i = 0; i < branches.size(); ++i) {
        ptr = (int*)(program+branches[i]+1);
        switch (*(program+branches[i])) {
            case BRANCH:
                (*ptr) = getInstructionBytecodeOffset(*ptr, instruction_count);
                break;
            case BRANCHIF:
                (*(ptr+1)) = getInstructionBytecodeOffset(*(ptr+1), instruction_count);
                (*(ptr+2)) = getInstructionBytecodeOffset(*(ptr+2), instruction_count);
                break;
        }
    }
}


Program& Program::istore(int_op regno, int_op i) {
    /*  Inserts istore instruction to bytecode.
     *
     *  :params:
     *
     *  regno:int - register number
     *  i:int     - value to store
     */
    ensurebytes(1 + 2*sizeof(bool) + 2*sizeof(int));

    bool regno_ref = false, i_ref = false;
    int regno_num, i_num;

    bool* boolptr = 0;
    int* intptr = 0;

    tie(regno_ref, regno_num) = regno;
    tie(i_ref, i_num) = i;

    program[addr_no++] = ISTORE;
    addr_ptr++;
    *((bool*)addr_ptr) = regno_ref;
    boolptr = (bool*)addr_ptr; boolptr++; addr_ptr = (char*)boolptr;
    *((int*)addr_ptr)  = regno_num;
    intptr = (int*)addr_ptr; intptr++; addr_ptr = (char*)intptr;
    *((bool*)addr_ptr) = i_ref;
    boolptr = (bool*)addr_ptr; boolptr++; addr_ptr = (char*)boolptr;
    *((int*)addr_ptr)  = i_num;
    intptr = (int*)addr_ptr; intptr++; addr_ptr = (char*)intptr;

    addr_no += 2*sizeof(bool) + 2*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::iadd(int_op rega, int_op regb, int_op regresult) {
    /*  Inserts iadd instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(int) + 3*sizeof(int));

    /*
    program[addr_no++] = IADD;
    addr_ptr++;
    ((int*)addr_ptr)[0] = rega;
    ((int*)addr_ptr)[1] = regb;
    ((int*)addr_ptr)[2] = regresult;
    addr_no += 3 * sizeof(int);
    addr_ptr = program+addr_no;
    */

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

Program& Program::ilt(int rega, int regb, int regresult) {
    /*  Inserts ilt instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(int));

    program[addr_no++] = ILT;
    addr_ptr++;
    ((int*)addr_ptr)[0] = rega;
    ((int*)addr_ptr)[1] = regb;
    ((int*)addr_ptr)[2] = regresult;
    addr_no += 3 * sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::ilte(int rega, int regb, int regresult) {
    /*  Inserts ilte instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(int));

    program[addr_no++] = ILTE;
    addr_ptr++;
    ((int*)addr_ptr)[0] = rega;
    ((int*)addr_ptr)[1] = regb;
    ((int*)addr_ptr)[2] = regresult;
    addr_no += 3 * sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::igt(int rega, int regb, int regresult) {
    /*  Inserts igt instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(int));

    program[addr_no++] = IGT;
    addr_ptr++;
    ((int*)addr_ptr)[0] = rega;
    ((int*)addr_ptr)[1] = regb;
    ((int*)addr_ptr)[2] = regresult;
    addr_no += 3 * sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::igte(int rega, int regb, int regresult) {
    /*  Inserts igte instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(int));

    program[addr_no++] = IGTE;
    addr_ptr++;
    ((int*)addr_ptr)[0] = rega;
    ((int*)addr_ptr)[1] = regb;
    ((int*)addr_ptr)[2] = regresult;
    addr_no += 3 * sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::ieq(int rega, int regb, int regresult) {
    /*  Inserts ieq instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(int));

    program[addr_no++] = IEQ;
    addr_ptr++;
    ((int*)addr_ptr)[0] = rega;
    ((int*)addr_ptr)[1] = regb;
    ((int*)addr_ptr)[2] = regresult;
    addr_no += 3 * sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::bstore(int regno, char b) {
    /*  Inserts bstore instruction to bytecode.
     *
     *  :params:
     *
     *  regno:int - register number
     *  i:int     - value to store
     */
    ensurebytes(1 + sizeof(int) + sizeof(char));

    program[addr_no++] = BSTORE;
    addr_ptr++;
    ((int*)addr_ptr)[0] = regno;
    addr_no += sizeof(int);
    addr_ptr = program+addr_no;
    addr_ptr[0] = b;
    addr_no += sizeof(char);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::print(int_op regno) {
    /*  Inserts print instuction.
     */
    ensurebytes(1 + sizeof(bool) + sizeof(int));

    bool regno_ref = false;
    int regno_num;

    bool* boolptr = 0;
    int* intptr = 0;

    tie(regno_ref, regno_num) = regno;

    program[addr_no++] = PRINT;
    addr_ptr++;
    *((bool*)addr_ptr) = regno_ref;
    boolptr = (bool*)addr_ptr; boolptr++; addr_ptr = (char*)boolptr;
    *((int*)addr_ptr)  = regno_num;
    intptr = (int*)addr_ptr; intptr++; addr_ptr = (char*)intptr;

    addr_no += sizeof(bool) + sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::echo(int_op regno) {
    /*  Inserts echo instuction.
     */
    ensurebytes(1 + sizeof(bool) + sizeof(int));

    bool regno_ref = false;
    int regno_num;

    bool* boolptr = 0;
    int* intptr = 0;

    tie(regno_ref, regno_num) = regno;

    program[addr_no++] = PRINT;
    addr_ptr++;
    *((bool*)addr_ptr) = regno_ref;
    boolptr = (bool*)addr_ptr; boolptr++; addr_ptr = (char*)boolptr;
    *((int*)addr_ptr)  = regno_num;
    intptr = (int*)addr_ptr; intptr++; addr_ptr = (char*)intptr;

    addr_no += sizeof(bool) + sizeof(int);
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

Program& Program::branchif(int regcondition, int addr_truth, int addr_false) {
    /*  Inserts branchif instruction.
     *  Byte offset is calculated automatically.
     *
     *  :params:
     *
     *  regcondition:int    - index of the instruction to which to branch
     *  addr_truth:int      - instruction index to go if condition is true
     *  addr_false:int      - instruction index to go if condition is false
     */
    ensurebytes(1 + 3*sizeof(int));

    // save branch instruction index for later evaluation
    branches.push_back(addr_no);

    program[addr_no++] = BRANCHIF;
    addr_ptr++;
    ((int*)addr_ptr)[0] = regcondition;
    ((int*)addr_ptr)[1] = addr_truth;
    ((int*)addr_ptr)[2] = addr_false;
    addr_no += 3*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::ret(int regno) {
    /*  Inserts ret instruction. Parameter is instruction index.
     *  Byte offset is calculated automatically.
     *
     *  :params:
     *
     *  regno:int   - index of the register which will be stored as return value
     */
    ensurebytes(1 + sizeof(int));

    program[addr_no++] = RET;
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
