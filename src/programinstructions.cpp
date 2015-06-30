#include "bytecode/opcodes.h"
#include "support/pointer.h"
#include "program.h"
using namespace std;


Program& Program::nop() {
    /*  Inserts nop instuction.
     */
    addr_ptr = cg::bytecode::nop(addr_ptr);
    return (*this);
}

Program& Program::izero(int_op regno) {
    /*  Inserts izero instuction.
     */
    addr_ptr = cg::bytecode::izero(addr_ptr, regno);
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
    addr_ptr = cg::bytecode::istore(addr_ptr, regno, i);
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
    addr_ptr = cg::bytecode::iadd(addr_ptr, rega, regb, regr);
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
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, ISUB, rega, regb, regr);
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
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IMUL, rega, regb, regr);
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
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IDIV, rega, regb, regr);
    return (*this);
}

Program& Program::iinc(int_op regno) {
    /*  Inserts iinc instuction.
     */
    *(addr_ptr++) = IINC;
    addr_ptr = insertIntegerOperand(addr_ptr, regno);
    return (*this);
}

Program& Program::idec(int_op regno) {
    /*  Inserts idec instuction.
     */
    *(addr_ptr++) = IDEC;
    addr_ptr = insertIntegerOperand(addr_ptr, regno);
    return (*this);
}

Program& Program::ilt(int_op rega, int_op regb, int_op regr) {
    /*  Inserts ilt instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, ILT, rega, regb, regr);
    return (*this);
}

Program& Program::ilte(int_op rega, int_op regb, int_op regr) {
    /*  Inserts ilte instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, ILTE, rega, regb, regr);
    return (*this);
}

Program& Program::igt(int_op rega, int_op regb, int_op regr) {
    /*  Inserts igt instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IGT, rega, regb, regr);
    return (*this);
}

Program& Program::igte(int_op rega, int_op regb, int_op regr) {
    /*  Inserts igte instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IGTE, rega, regb, regr);
    return (*this);
}

Program& Program::ieq(int_op rega, int_op regb, int_op regr) {
    /*  Inserts ieq instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, IEQ, rega, regb, regr);
    return (*this);
}

Program& Program::fstore(int_op regno, float f) {
    /*  Inserts fstore instruction to bytecode.
     *
     *  :params:
     *
     *  regno - register number
     *  f     - value to store
     */
    *(addr_ptr++) = FSTORE;
    addr_ptr = insertIntegerOperand(addr_ptr, regno);
    *((float*)addr_ptr)  = f;
    pointer::inc<float, byte>(addr_ptr);

    return (*this);
}

Program& Program::fadd(int_op rega, int_op regb, int_op regr) {
    /*  Inserts fadd instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FADD, rega, regb, regr);
    return (*this);
}

Program& Program::fsub(int_op rega, int_op regb, int_op regr) {
    /*  Inserts fsub instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FSUB, rega, regb, regr);
    return (*this);
}

Program& Program::fmul(int_op rega, int_op regb, int_op regr) {
    /*  Inserts fmul instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FMUL, rega, regb, regr);
    return (*this);
}

Program& Program::fdiv(int_op rega, int_op regb, int_op regr) {
    /*  Inserts fdiv instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FDIV, rega, regb, regr);
    return (*this);
}

Program& Program::flt(int_op rega, int_op regb, int_op regr) {
    /*  Inserts flt instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FLT, rega, regb, regr);
    return (*this);
}

Program& Program::flte(int_op rega, int_op regb, int_op regr) {
    /*  Inserts flte instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FLTE, rega, regb, regr);
    return (*this);
}

Program& Program::fgt(int_op rega, int_op regb, int_op regr) {
    /*  Inserts fgt instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the resugt
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FGT, rega, regb, regr);
    return (*this);
}

Program& Program::fgte(int_op rega, int_op regb, int_op regr) {
    /*  Inserts fgte instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the resugt
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FGTE, rega, regb, regr);
    return (*this);
}

Program& Program::feq(int_op rega, int_op regb, int_op regr) {
    /*  Inserts feq instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the resugt
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, FEQ, rega, regb, regr);
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
    bool b_ref = false;
    byte bt;

    tie(b_ref, bt) = b;

    *(addr_ptr++) = BSTORE;
    addr_ptr = insertIntegerOperand(addr_ptr, regno);
    *((bool*)addr_ptr) = b_ref;
    pointer::inc<bool, byte>(addr_ptr);
    *((byte*)addr_ptr)  = bt;
    ++addr_ptr;

    return (*this);
}

Program& Program::itof(int_op a, int_op b) {
    /*  Inserts itof instruction to bytecode.
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, ITOF, a, b);
    return (*this);
}

Program& Program::ftoi(int_op a, int_op b) {
    /*  Inserts ftoi instruction to bytecode.
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, FTOI, a, b);
    return (*this);
}

Program& Program::stoi(int_op a, int_op b) {
    /*  Inserts stoi instruction to bytecode.
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, STOI, a, b);
    return (*this);
}

Program& Program::stof(int_op a, int_op b) {
    /*  Inserts stof instruction to bytecode.
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, STOF, a, b);
    return (*this);
}

Program& Program::strstore(int_op reg, string s) {
    /*  Inserts strstore instruction.
     */
    *(addr_ptr++) = STRSTORE;
    addr_ptr = insertIntegerOperand(addr_ptr, reg);

    for (unsigned i = 1; i < s.size()-1; ++i) {
        *((char*)addr_ptr++) = s[i];
    }
    *((char*)addr_ptr++) = char(0);
    return (*this);
}

Program& Program::vec(int_op index) {
    /** Inserts vec instruction.
     */
    *(addr_ptr++) = VEC;
    addr_ptr = insertIntegerOperand(addr_ptr, index);
    return (*this);
}

Program& Program::vinsert(int_op vec, int_op src, int_op dst) {
    /** Inserts vinsert instruction.
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, VINSERT, vec, src, dst);
    return (*this);
}

Program& Program::vpush(int_op vec, int_op src) {
    /** Inserts vpush instruction.
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, VPUSH, vec, src);
    return (*this);
}

Program& Program::vpop(int_op vec, int_op dst, int_op pos) {
    /** Inserts vpop instruction.
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, VPOP, vec, dst, pos);
    return (*this);
}

Program& Program::vat(int_op vec, int_op dst, int_op at) {
    /** Inserts vat instruction.
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, VAT, vec, dst, at);
    return (*this);
}

Program& Program::vlen(int_op vec, int_op reg) {
    /** Inserts vlen instruction.
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, VLEN, vec, reg);
    return (*this);
}

Program& Program::lognot(int_op reg) {
    /*  Inserts not instuction.
     */
    *(addr_ptr++) = NOT;
    addr_ptr = insertIntegerOperand(addr_ptr, reg);
    return (*this);
}

Program& Program::logand(int_op rega, int_op regb, int_op regr) {
    /*  Inserts and instruction to bytecode.
     *
     *  :params:
     *
     *  rega   - register index of first operand
     *  regb   - register index of second operand
     *  regr   - register index in which to store the result
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, AND, rega, regb, regr);
    return (*this);
}

Program& Program::logor(int_op rega, int_op regb, int_op regr) {
    /*  Inserts or instruction to bytecode.
     *
     *  :params:
     *
     *  rega   - register index of first operand
     *  regb   - register index of second operand
     *  regr   - register index in which to store the result
     */
    addr_ptr = insertThreeIntegerOpsInstruction(addr_ptr, OR, rega, regb, regr);
    return (*this);
}

Program& Program::move(int_op a, int_op b) {
    /*  Inserts move instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number (move from...)
     *  b - register number (move to...)
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, MOVE, a, b);
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
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, COPY, a, b);
    return (*this);
}

Program& Program::ref(int_op a, int_op b) {
    /*  Inserts ref instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number
     *  b - register number
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, REF, a, b);
    return (*this);
}

Program& Program::swap(int_op a, int_op b) {
    /*  Inserts swap instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number
     *  b - register number
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, SWAP, a, b);
    return (*this);
}

Program& Program::free(int_op reg) {
    /*  Inserts free instuction.
     */
    *(addr_ptr++) = FREE;
    addr_ptr = insertIntegerOperand(addr_ptr, reg);
    return (*this);
}

Program& Program::empty(int_op reg) {
    /*  Inserts empty instuction.
     */
    *(addr_ptr++) = EMPTY;
    addr_ptr = insertIntegerOperand(addr_ptr, reg);
    return (*this);
}

Program& Program::isnull(int_op a, int_op b) {
    /*  Inserts isnull instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number
     *  b - register number
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, ISNULL, a, b);
    return (*this);
}

Program& Program::ress(string a) {
    /*  Inserts ress instruction to bytecode.
     *
     *  :params:
     *
     *  a - register set ID
     */
    *(addr_ptr++) = RESS;
    if (a == "global") {
        *((int*)addr_ptr) = 0;
    } else if (a == "local") {
        *((int*)addr_ptr) = 1;
    } else if (a == "static") {
        *((int*)addr_ptr) = 2;
    } else if (a == "temp") {
        *((int*)addr_ptr) = 3;
    }
    pointer::inc<int, byte>(addr_ptr);
    return (*this);
}

Program& Program::tmpri(int_op reg) {
    /*  Inserts tmpri instuction.
     */
    *(addr_ptr++) = TMPRI;
    addr_ptr = insertIntegerOperand(addr_ptr, reg);
    return (*this);
}

Program& Program::tmpro(int_op reg) {
    /*  Inserts tmpro instuction.
     */
    *(addr_ptr++) = TMPRO;
    addr_ptr = insertIntegerOperand(addr_ptr, reg);
    return (*this);
}

Program& Program::print(int_op reg) {
    /*  Inserts print instuction.
     */
    *(addr_ptr++) = PRINT;
    addr_ptr = insertIntegerOperand(addr_ptr, reg);
    return (*this);
}

Program& Program::echo(int_op reg) {
    /*  Inserts echo instuction.
     */
    *(addr_ptr++) = ECHO;
    addr_ptr = insertIntegerOperand(addr_ptr, reg);
    return (*this);
}

Program& Program::clbind(int_op reg) {
    /*  Inserts clbing instuction.
     */
    *(addr_ptr++) = CLBIND;
    addr_ptr = insertIntegerOperand(addr_ptr, reg);
    return (*this);
}

Program& Program::closure(string fn, int_op reg) {
    /*  Inserts closure instuction.
     */
    *(addr_ptr++) = CLOSURE;
    for (unsigned i = 0; i < fn.size(); ++i) {
        *((char*)addr_ptr++) = fn[i];
    }
    *(addr_ptr++) = '\0';
    addr_ptr = insertIntegerOperand(addr_ptr, reg);
    return (*this);
}

Program& Program::function(string fn, int_op reg) {
    /*  Inserts function instuction.
     */
    *(addr_ptr++) = FUNCTION;
    for (unsigned i = 0; i < fn.size(); ++i) {
        *((char*)addr_ptr++) = fn[i];
    }
    *(addr_ptr++) = '\0';
    addr_ptr = insertIntegerOperand(addr_ptr, reg);
    return (*this);
}

Program& Program::fcall(int_op clsr, int_op ret) {
    /*  Inserts fcall instruction to bytecode.
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, FCALL, clsr, ret);
    return (*this);
}

Program& Program::frame(int_op a, int_op b) {
    /*  Inserts frame instruction to bytecode.
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, FRAME, a, b);
    return (*this);
}

Program& Program::param(int_op a, int_op b) {
    /*  Inserts param instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number
     *  b - register number
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, PARAM, a, b);
    return (*this);
}

Program& Program::paref(int_op a, int_op b) {
    /*  Inserts paref instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number
     *  b - register number
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, PAREF, a, b);
    return (*this);
}

Program& Program::arg(int_op a, int_op b) {
    /*  Inserts arg instruction to bytecode.
     *
     *  :params:
     *
     *  a - argument number
     *  b - register number
     */
    addr_ptr = insertTwoIntegerOpsInstruction(addr_ptr, ARG, a, b);
    return (*this);
}

Program& Program::call(string fn_name, int_op reg) {
    /*  Inserts call instruction.
     *  Byte offset is calculated automatically.
     */
    *(addr_ptr++) = CALL;
    for (unsigned i = 0; i < fn_name.size(); ++i) {
        *((char*)addr_ptr++) = fn_name[i];
    }
    *(addr_ptr++) = '\0';
    addr_ptr = insertIntegerOperand(addr_ptr, reg);
    return (*this);
}

Program& Program::jump(int addr, enum JUMPTYPE is_absolute) {
    /*  Inserts jump instruction. Parameter is instruction index.
     *  Byte offset is calculated automatically.
     *
     *  :params:
     *
     *  addr:int    - index of the instruction to which to branch
     */
    *(addr_ptr++) = JUMP;

    // save jump position if jump is not to byte
    if (is_absolute != JMP_TO_BYTE) {
        (is_absolute == JMP_ABSOLUTE ? branches_absolute : branches).push_back(addr_ptr);
    }
    *((int*)addr_ptr) = addr;
    pointer::inc<int, byte>(addr_ptr);

    return (*this);
}

Program& Program::branch(int_op regc, int addr_truth, enum JUMPTYPE absolute_truth, int addr_false, enum JUMPTYPE absolute_false) {
    /*  Inserts branch instruction.
     *  Byte offset is calculated automatically.
     */

    *(addr_ptr++) = BRANCH;
    addr_ptr = insertIntegerOperand(addr_ptr, regc);

    // save jump position if jump is not to byte
    if (absolute_truth != JMP_TO_BYTE) {
        (absolute_truth == JMP_ABSOLUTE ? branches_absolute : branches).push_back(addr_ptr);
    }
    *((int*)addr_ptr) = addr_truth;
    pointer::inc<int, byte>(addr_ptr);

    // save jump position if jump is not to byte
    if (absolute_false != JMP_TO_BYTE) {
        (absolute_truth == JMP_ABSOLUTE ? branches_absolute : branches).push_back(addr_ptr);
    }
    *((int*)addr_ptr) = addr_false;
    pointer::inc<int, byte>(addr_ptr);

    return (*this);
}

Program& Program::tryframe() {
    /*  Inserts tryframe instruction.
     */
    *(addr_ptr++) = TRYFRAME;
    return (*this);
}

Program& Program::vmcatch(string type_name, string block_name) {
    /*  Inserts catch instruction.
     */
    *(addr_ptr++) = CATCH;

    // the type
    for (unsigned i = 1; i < type_name.size()-1; ++i) {
        *((char*)addr_ptr++) = type_name[i];
    }
    *((char*)addr_ptr++) = '\0';

    // catcher block name
    for (unsigned i = 0; i < block_name.size(); ++i) {
        *((char*)addr_ptr++) = block_name[i];
    }
    *((char*)addr_ptr++) = '\0';

    return (*this);
}

Program& Program::pull(int_op regno) {
    /*  Inserts throw instuction.
     */
    *(addr_ptr++) = PULL;
    addr_ptr = insertIntegerOperand(addr_ptr, regno);
    return (*this);
}

Program& Program::vmtry(string block_name) {
    /*  Inserts try instruction.
     *  Byte offset is calculated automatically.
     */
    *(addr_ptr++) = TRY;
    for (unsigned i = 0; i < block_name.size(); ++i) {
        *((char*)addr_ptr++) = block_name[i];
    }
    *(addr_ptr++) = '\0';
    return (*this);
}

Program& Program::vmthrow(int_op regno) {
    /*  Inserts throw instuction.
     */
    *(addr_ptr++) = THROW;
    addr_ptr = insertIntegerOperand(addr_ptr, regno);
    return (*this);
}

Program& Program::leave() {
    /*  Inserts leave instruction.
     */
    *(addr_ptr++) = LEAVE;
    return (*this);
}

Program& Program::eximport(string module_name) {
    /*  Inserts eximport instruction.
     */
    *(addr_ptr++) = EXIMPORT;
    for (unsigned i = 1; i < module_name.size()-1; ++i) {
        *((char*)addr_ptr++) = module_name[i];
    }
    *((char*)addr_ptr++) = char(0);
    return (*this);
}

Program& Program::excall(string fn_name, int_op reg) {
    /*  Inserts excall instruction.
     *  Byte offset is calculated automatically.
     */
    *(addr_ptr++) = EXCALL;
    for (unsigned i = 0; i < fn_name.size(); ++i) {
        *((char*)addr_ptr++) = fn_name[i];
    }
    *(addr_ptr++) = '\0';
    addr_ptr = insertIntegerOperand(addr_ptr, reg);
    return (*this);
}

Program& Program::end() {
    /*  Inserts end instruction.
     */
    *(addr_ptr++) = END;
    return (*this);
}

Program& Program::halt() {
    /*  Inserts halt instruction.
     */
    *(addr_ptr++) = HALT;
    return (*this);
}
