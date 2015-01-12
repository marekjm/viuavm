#include <iostream>
#include <string>
#include <sstream>
#include "bytecode/bytetypedef.h"
#include "bytecode/opcodes.h"
#include "bytecode/maps.h"
#include "support/pointer.h"
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


Program& Program::setdebug(bool d) {
    /** Sets debugging status.
     */
    debug = d;
    return (*this);
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
            case BRANCH:
                i += 3 * sizeof(int);
                break;
            case ISTORE:
                i += 2 * sizeof(int);
                break;
            case IINC:
            case IDEC:
            case PRINT:
            case JUMP:
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


int Program::getInstructionBytecodeOffset(int instr, int count) {
    /** Returns bytecode offset for given instruction index.
     *  The "count" parameter is there to pass assumed instruction count to
     *  avoid recalculating it on every call.
     */

    // check if instruction count was passed, and calculate it if not
    count = (count >= 0 ? count : instructionCount());

    int offset = 0;
    int inc;
    for (int i = 0; i < (instr >= 0 ? instr : count+instr); ++i) {
        /*  This loop iterates over so many instructions as needed to find bytecode offset for requested instruction.
         *
         *  Each time, the offset is increased by `inc` - which is equal to *1 plus size of operands of instructions at current index*.
         */
        inc = 1;
        if (debug) { cout << "increasing instruction offset (" << i+1 << '/' << (instr >= 0 ? instr : count+instr) << "): "; }
        switch (program[offset]) {
            case IADD:
                if (debug) { cout << "iadd"; }
                inc += 3*sizeof(bool) + 3*sizeof(int);
                break;
            case ISUB:
                if (debug) { cout << "isub"; }
                inc += 3*sizeof(bool) + 3*sizeof(int);
                break;
            case IMUL:
                if (debug) { cout << "imul"; }
                inc += 3*sizeof(bool) + 3*sizeof(int);
                break;
            case IDIV:
                if (debug) { cout << "idiv"; }
                inc += 3*sizeof(bool) + 3*sizeof(int);
                break;
            case ILT:
                if (debug) { cout << "ilt"; }
                inc += 3*sizeof(bool) + 3*sizeof(int);
                break;
            case ILTE:
                if (debug) { cout << "ilte"; }
                inc += 3*sizeof(bool) + 3*sizeof(int);
                break;
            case IGT:
                if (debug) { cout << "igt"; }
                inc += 3*sizeof(bool) + 3*sizeof(int);
                break;
            case IGTE:
                if (debug) { cout << "igte"; }
                inc += 3*sizeof(bool) + 3*sizeof(int);
                break;
            case IEQ:
                if (debug) { cout << "ieq"; }
                inc += 3*sizeof(bool) + 3*sizeof(int);
                break;
            case BRANCH:
                if (debug) { cout << "branch"; }
                inc += sizeof(bool) + 3*sizeof(int);
                break;
            case ISTORE:
                if (debug) { cout << "istore"; }
                inc += 2*sizeof(bool) + 2*sizeof(int);
                break;
            case IINC:
                if (debug) { cout << "iinc"; }
                inc += sizeof(bool) + sizeof(int);
                break;
            case IDEC:
                if (debug) { cout << "idec"; }
                inc += sizeof(bool) + sizeof(int);
                break;
            case NOT:
                if (debug) { cout << "not"; }
                inc += sizeof(bool) + sizeof(int);
                break;
            case AND:
                if (debug) { cout << "and"; }
                inc += 3*sizeof(bool) + 3*sizeof(int);
                break;
            case OR:
                if (debug) { cout << "or"; }
                inc += 3*sizeof(bool) + 3*sizeof(int);
                break;
            case PRINT:
                if (debug) { cout << "print"; }
                inc += sizeof(bool) + sizeof(int);
                break;
            case JUMP:
                if (debug) { cout << "jump"; }
                inc += sizeof(int);
                break;
            case RET:
                if (debug) { cout << "ret"; }
                inc += sizeof(bool) + sizeof(int);
                break;
        }
        if (debug) {
            cout << ": " << inc;
            cout << endl;
        }
        offset += inc;
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
    for (unsigned i = 0; i < branches.size(); ++i) {
        ptr = (int*)(program+branches[i]+1);
        switch (*(program+branches[i])) {
            case JUMP:
                (*ptr) = getInstructionBytecodeOffset(*ptr, instruction_count);
                break;
            case BRANCH:
                pointer::inc<bool, int>(ptr);
                if (debug) { cout << "calculating branch:  true: " << *(ptr+1) << endl; }
                (*(ptr+1)) = getInstructionBytecodeOffset(*(ptr+1), instruction_count);
                if (debug) { cout << "calculated branch:   true: " << *(ptr+1) << endl; }

                if (debug) { cout << "calculating branch: false: " << *(ptr+2) << endl; }
                (*(ptr+2)) = getInstructionBytecodeOffset(*(ptr+2), instruction_count);
                if (debug) { cout << "calculated branch:  false: " << *(ptr+2) << endl; }
                break;
        }
    }

    return (*this);
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

    tie(regno_ref, regno_num) = regno;
    tie(i_ref, i_num) = i;

    program[addr_no++] = ISTORE;
    addr_ptr++;

    *((bool*)addr_ptr) = regno_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = regno_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = i_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = i_num;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += 2*sizeof(bool) + 2*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::iadd(int_op rega, int_op regb, int_op regr) {
    /*  Inserts iadd instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(bool) + 3*sizeof(int));

    bool rega_ref = false, regb_ref = false, regr_ref = false;
    int rega_num, regb_num, regr_num;

    tie(rega_ref, rega_num) = rega;
    tie(regb_ref, regb_num) = regb;
    tie(regr_ref, regr_num) = regr;

    program[addr_no++] = IADD;
    addr_ptr++;

    *((bool*)addr_ptr) = rega_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = rega_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regb_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = regb_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regr_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = regr_num;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += 3*sizeof(bool) + 3*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::isub(int_op rega, int_op regb, int_op regr) {
    /*  Inserts isub instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(bool) + 3*sizeof(int));

    bool rega_ref = false, regb_ref = false, regr_ref = false;
    int rega_num, regb_num, regr_num;

    tie(rega_ref, rega_num) = rega;
    tie(regb_ref, regb_num) = regb;
    tie(regr_ref, regr_num) = regr;

    program[addr_no++] = ISUB;
    addr_ptr++;

    *((bool*)addr_ptr) = rega_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = rega_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regb_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = regb_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regr_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = regr_num;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += 3*sizeof(bool) + 3*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::imul(int_op rega, int_op regb, int_op regr) {
    /*  Inserts imul instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(bool) + 3*sizeof(int));

    bool rega_ref = false, regb_ref = false, regr_ref = false;
    int rega_num, regb_num, regr_num;

    tie(rega_ref, rega_num) = rega;
    tie(regb_ref, regb_num) = regb;
    tie(regr_ref, regr_num) = regr;

    program[addr_no++] = IMUL;
    addr_ptr++;

    *((bool*)addr_ptr) = rega_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = rega_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regb_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = regb_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regr_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = regr_num;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += 3*sizeof(bool) + 3*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::idiv(int_op rega, int_op regb, int_op regr) {
    /*  Inserts idiv instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(bool) + 3*sizeof(int));

    bool rega_ref = false, regb_ref = false, regr_ref = false;
    int rega_num, regb_num, regr_num;

    tie(rega_ref, rega_num) = rega;
    tie(regb_ref, regb_num) = regb;
    tie(regr_ref, regr_num) = regr;

    program[addr_no++] = IDIV;
    addr_ptr++;

    *((bool*)addr_ptr) = rega_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = rega_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regb_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = regb_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regr_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = regr_num;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += 3*sizeof(bool) + 3*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::iinc(int_op regno) {
    /*  Inserts iinc instuction.
     */
    ensurebytes(1 + sizeof(bool) + sizeof(int));

    bool regno_ref = false;
    int regno_num;

    tie(regno_ref, regno_num) = regno;

    program[addr_no++] = IINC;
    addr_ptr++;
    *((bool*)addr_ptr) = regno_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = regno_num;
    pointer::inc<bool, byte>(addr_ptr);

    addr_no += sizeof(bool) + sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::idec(int_op regno) {
    /*  Inserts idec instuction.
     */
    ensurebytes(1 + sizeof(bool) + sizeof(int));

    bool regno_ref = false;
    int regno_num;

    tie(regno_ref, regno_num) = regno;

    program[addr_no++] = IDEC;
    addr_ptr++;
    *((bool*)addr_ptr) = regno_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = regno_num;
    pointer::inc<bool, byte>(addr_ptr);

    addr_no += sizeof(bool) + sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::ilt(int_op rega, int_op regb, int_op regresult) {
    /*  Inserts ilt instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(bool) + 3*sizeof(int));

    bool rega_ref, regb_ref, regresult_ref;
    int  rega_num, regb_num, regresult_num;

    tie(rega_ref, rega_num) = rega;
    tie(regb_ref, regb_num) = regb;
    tie(regresult_ref, regresult_num) = regresult;

    program[addr_no++] = ILT;
    addr_ptr++;

    *((bool*)addr_ptr) = rega_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = rega_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regb_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regb_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regresult_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regresult_num;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += 3*sizeof(bool) + 3*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::ilte(int_op rega, int_op regb, int_op regresult) {
    /*  Inserts ilte instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(bool) + 3*sizeof(int));

    bool rega_ref, regb_ref, regresult_ref;
    int  rega_num, regb_num, regresult_num;

    tie(rega_ref, rega_num) = rega;
    tie(regb_ref, regb_num) = regb;
    tie(regresult_ref, regresult_num) = regresult;

    program[addr_no++] = ILTE;
    addr_ptr++;

    *((bool*)addr_ptr) = rega_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = rega_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regb_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regb_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regresult_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regresult_num;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += 3*sizeof(bool) + 3*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::igt(int_op rega, int_op regb, int_op regresult) {
    /*  Inserts igt instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(bool) + 3*sizeof(int));

    bool rega_ref, regb_ref, regresult_ref;
    int  rega_num, regb_num, regresult_num;

    tie(rega_ref, rega_num) = rega;
    tie(regb_ref, regb_num) = regb;
    tie(regresult_ref, regresult_num) = regresult;

    program[addr_no++] = IGT;
    addr_ptr++;

    *((bool*)addr_ptr) = rega_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = rega_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regb_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regb_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regresult_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regresult_num;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += 3*sizeof(bool) + 3*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::igte(int_op rega, int_op regb, int_op regresult) {
    /*  Inserts igte instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(bool) + 3*sizeof(int));

    bool rega_ref, regb_ref, regresult_ref;
    int  rega_num, regb_num, regresult_num;

    tie(rega_ref, rega_num) = rega;
    tie(regb_ref, regb_num) = regb;
    tie(regresult_ref, regresult_num) = regresult;

    program[addr_no++] = IGTE;
    addr_ptr++;

    *((bool*)addr_ptr) = rega_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = rega_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regb_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regb_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regresult_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regresult_num;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += 3*sizeof(bool) + 3*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::ieq(int_op rega, int_op regb, int_op regresult) {
    /*  Inserts ieq instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regresult:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(bool) + 3*sizeof(int));

    bool rega_ref, regb_ref, regresult_ref;
    int  rega_num, regb_num, regresult_num;

    tie(rega_ref, rega_num) = rega;
    tie(regb_ref, regb_num) = regb;
    tie(regresult_ref, regresult_num) = regresult;

    program[addr_no++] = IEQ;
    addr_ptr++;

    *((bool*)addr_ptr) = rega_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = rega_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regb_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regb_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regresult_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regresult_num;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += 3*sizeof(bool) + 3*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::bstore(int_op regno, byte_op b) {
    /*  Inserts bstore instruction to bytecode.
     *
     *  :params:
     *
     *  regno - register number
     *  b     - value to store
     */
    ensurebytes(1 + 2*sizeof(bool) + sizeof(int) + sizeof(byte));


    bool regno_ref = false, b_ref = false;
    int regno_num;
    byte bt;

    tie(regno_ref, regno_num) = regno;
    tie(b_ref, bt) = b;

    program[addr_no++] = BSTORE;
    addr_ptr++;

    *((bool*)addr_ptr) = regno_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = regno_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = b_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((byte*)addr_ptr)  = bt;
    ++addr_ptr;

    addr_no += 2*sizeof(bool) + sizeof(int) + sizeof(byte);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::lognot(int_op reg) {
    /*  Inserts not instuction.
     */
    ensurebytes(1 + sizeof(bool) + sizeof(int));

    bool reg_ref = false;
    int reg_num;

    tie(reg_ref, reg_num) = reg;

    program[addr_no++] = NOT;
    addr_ptr++;
    *((bool*)addr_ptr) = reg_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = reg_num;
    pointer::inc<bool, byte>(addr_ptr);

    addr_no += sizeof(bool) + sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::logand(int_op rega, int_op regb, int_op regr) {
    /*  Inserts and instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regr:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(bool) + 3*sizeof(int));

    bool rega_ref, regb_ref, regr_ref;
    int  rega_num, regb_num, regr_num;

    tie(rega_ref, rega_num) = rega;
    tie(regb_ref, regb_num) = regb;
    tie(regr_ref, regr_num) = regr;

    program[addr_no++] = AND;
    addr_ptr++;

    *((bool*)addr_ptr) = rega_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = rega_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regb_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regb_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regr_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regr_num;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += 3*sizeof(bool) + 3*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::logor(int_op rega, int_op regb, int_op regr) {
    /*  Inserts or instruction to bytecode.
     *
     *  :params:
     *
     *  rega:int        - register index of first operand
     *  regb:int        - register index of second operand
     *  regr:int   - register index in which to store the result
     */
    ensurebytes(1 + 3*sizeof(bool) + 3*sizeof(int));

    bool rega_ref, regb_ref, regr_ref;
    int  rega_num, regb_num, regr_num;

    tie(rega_ref, rega_num) = rega;
    tie(regb_ref, regb_num) = regb;
    tie(regr_ref, regr_num) = regr;

    program[addr_no++] = OR;
    addr_ptr++;

    *((bool*)addr_ptr) = rega_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = rega_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regb_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regb_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = regr_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regr_num;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += 3*sizeof(bool) + 3*sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::copy(int_op a, int_op b) {
    /*  Inserts copy instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number (copy from...)
     *  b - register number (copy to...)
     */
    ensurebytes(1 + 2*sizeof(bool) + sizeof(int) + sizeof(byte));


    bool a_ref = false, b_ref = false;
    int a_num, b_num;

    tie(a_ref, a_num) = a;
    tie(b_ref, b_num) = b;

    program[addr_no++] = COPY;
    addr_ptr++;

    *((bool*)addr_ptr) = a_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = a_num;
    pointer::inc<int, byte>(addr_ptr);

    *((bool*)addr_ptr) = b_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr)  = b_num;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += 2*sizeof(bool) + 2*sizeof(int);
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
    boolptr = (bool*)addr_ptr; boolptr++; addr_ptr = (byte*)boolptr;
    *((int*)addr_ptr)  = regno_num;
    intptr = (int*)addr_ptr; intptr++; addr_ptr = (byte*)intptr;

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

    program[addr_no++] = ECHO;
    addr_ptr++;
    *((bool*)addr_ptr) = regno_ref;
    boolptr = (bool*)addr_ptr; boolptr++; addr_ptr = (byte*)boolptr;
    *((int*)addr_ptr)  = regno_num;
    intptr = (int*)addr_ptr; intptr++; addr_ptr = (byte*)intptr;

    addr_no += sizeof(bool) + sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::jump(int addr) {
    /*  Inserts jump instruction. Parameter is instruction index.
     *  Byte offset is calculated automatically.
     *
     *  :params:
     *
     *  addr:int    - index of the instruction to which to branch
     */
    ensurebytes(1 + sizeof(int));

    // save branch instruction index for later evaluation
    branches.push_back(addr_no);

    program[addr_no++] = JUMP;
    addr_ptr++;

    *((int*)addr_ptr) = addr;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += sizeof(int);
    addr_ptr = program+addr_no;

    return (*this);
}

Program& Program::branch(int_op regc, int addr_truth, int addr_false) {
    /*  Inserts branch instruction.
     *  Byte offset is calculated automatically.
     *
     *  :params:
     *
     *  regc:int    - index of the instruction to which to branch
     *  addr_truth:int      - instruction index to go if condition is true
     *  addr_false:int      - instruction index to go if condition is false
     */
    ensurebytes(1 + sizeof(bool) + 3*sizeof(int));

    // save branch instruction index for later evaluation
    branches.push_back(addr_no);

    bool regcond_ref;
    int regcond_num;

    tie(regcond_ref, regcond_num) = regc;

    program[addr_no++] = BRANCH;
    addr_ptr++;

    *((bool*)addr_ptr) = regcond_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((int*)addr_ptr) = regcond_num;
    pointer::inc<int, byte>(addr_ptr);

    *((int*)addr_ptr) = addr_truth;
    pointer::inc<int, byte>(addr_ptr);
    *((int*)addr_ptr) = addr_false;
    pointer::inc<int, byte>(addr_ptr);

    addr_no += sizeof(bool) + 3*sizeof(int);
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
