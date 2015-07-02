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
    addr_ptr = cg::bytecode::isub(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::imul(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::idiv(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::iinc(int_op regno) {
    /*  Inserts iinc instuction.
     */
    addr_ptr = cg::bytecode::iinc(addr_ptr, regno);
    return (*this);
}

Program& Program::idec(int_op regno) {
    /*  Inserts idec instuction.
     */
    addr_ptr = cg::bytecode::idec(addr_ptr, regno);
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
    addr_ptr = cg::bytecode::ilt(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::ilte(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::igt(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::igte(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::ieq(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::fstore(addr_ptr, regno, f);
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
    addr_ptr = cg::bytecode::fadd(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::fsub(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::fmul(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::fdiv(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::flt(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::flte(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::fgt(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::fgte(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::feq(addr_ptr, rega, regb, regr);
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
    addr_ptr = cg::bytecode::bstore(addr_ptr, regno, b);
    return (*this);
}

Program& Program::itof(int_op a, int_op b) {
    /*  Inserts itof instruction to bytecode.
     */
    addr_ptr = cg::bytecode::itof(addr_ptr, a, b);
    return (*this);
}

Program& Program::ftoi(int_op a, int_op b) {
    /*  Inserts ftoi instruction to bytecode.
     */
    addr_ptr = cg::bytecode::ftoi(addr_ptr, a, b);
    return (*this);
}

Program& Program::stoi(int_op a, int_op b) {
    /*  Inserts stoi instruction to bytecode.
     */
    addr_ptr = cg::bytecode::stoi(addr_ptr, a, b);
    return (*this);
}

Program& Program::stof(int_op a, int_op b) {
    /*  Inserts stof instruction to bytecode.
     */
    addr_ptr = cg::bytecode::stof(addr_ptr, a, b);
    return (*this);
}

Program& Program::strstore(int_op reg, string s) {
    /*  Inserts strstore instruction.
     */
    addr_ptr = cg::bytecode::strstore(addr_ptr, reg, s);
    return (*this);
}

Program& Program::vec(int_op index) {
    /** Inserts vec instruction.
     */
    addr_ptr = cg::bytecode::vec(addr_ptr, index);
    return (*this);
}

Program& Program::vinsert(int_op vec, int_op src, int_op dst) {
    /** Inserts vinsert instruction.
     */
    addr_ptr = cg::bytecode::vinsert(addr_ptr, vec, src, dst);
    return (*this);
}

Program& Program::vpush(int_op vec, int_op src) {
    /** Inserts vpush instruction.
     */
    addr_ptr = cg::bytecode::vpush(addr_ptr, vec, src);
    return (*this);
}

Program& Program::vpop(int_op vec, int_op dst, int_op pos) {
    /** Inserts vpop instruction.
     */
    addr_ptr = cg::bytecode::vpop(addr_ptr, vec, dst, pos);
    return (*this);
}

Program& Program::vat(int_op vec, int_op dst, int_op at) {
    /** Inserts vat instruction.
     */
    addr_ptr = cg::bytecode::vat(addr_ptr, vec, dst, at);
    return (*this);
}

Program& Program::vlen(int_op vec, int_op reg) {
    /** Inserts vlen instruction.
     */
    addr_ptr = cg::bytecode::vlen(addr_ptr, vec, reg);
    return (*this);
}

Program& Program::lognot(int_op reg) {
    /*  Inserts not instuction.
     */
    addr_ptr = cg::bytecode::lognot(addr_ptr, reg);
    return (*this);
}

Program& Program::logand(int_op regr, int_op rega, int_op regb) {
    /*  Inserts and instruction to bytecode.
     *
     *  :params:
     *
     *  regr   - register index in which to store the result
     *  rega   - register index of first operand
     *  regb   - register index of second operand
     */
    addr_ptr = cg::bytecode::logand(addr_ptr, regr, rega, regb);
    return (*this);
}

Program& Program::logor(int_op regr, int_op rega, int_op regb) {
    /*  Inserts or instruction to bytecode.
     *
     *  :params:
     *
     *  regr   - register index in which to store the result
     *  rega   - register index of first operand
     *  regb   - register index of second operand
     */
    addr_ptr = cg::bytecode::logor(addr_ptr, regr, rega, regb);
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
    addr_ptr = cg::bytecode::move(addr_ptr, a, b);
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
    addr_ptr = cg::bytecode::copy(addr_ptr, a, b);
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
    addr_ptr = cg::bytecode::ref(addr_ptr, a, b);
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
    addr_ptr = cg::bytecode::swap(addr_ptr, a, b);
    return (*this);
}

Program& Program::free(int_op reg) {
    /*  Inserts free instuction.
     */
    addr_ptr = cg::bytecode::free(addr_ptr, reg);
    return (*this);
}

Program& Program::empty(int_op reg) {
    /*  Inserts empty instuction.
     */
    addr_ptr = cg::bytecode::empty(addr_ptr, reg);
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
    addr_ptr = cg::bytecode::isnull(addr_ptr, a, b);
    return (*this);
}

Program& Program::ress(string a) {
    /*  Inserts ress instruction to bytecode.
     *
     *  :params:
     *
     *  a - register set ID
     */
    addr_ptr = cg::bytecode::ress(addr_ptr, a);
    return (*this);
}

Program& Program::tmpri(int_op reg) {
    /*  Inserts tmpri instuction.
     */
    addr_ptr = cg::bytecode::tmpri(addr_ptr, reg);
    return (*this);
}

Program& Program::tmpro(int_op reg) {
    /*  Inserts tmpro instuction.
     */
    addr_ptr = cg::bytecode::tmpro(addr_ptr, reg);
    return (*this);
}

Program& Program::print(int_op reg) {
    /*  Inserts print instuction.
     */
    addr_ptr = cg::bytecode::print(addr_ptr, reg);
    return (*this);
}

Program& Program::echo(int_op reg) {
    /*  Inserts echo instuction.
     */
    addr_ptr = cg::bytecode::echo(addr_ptr, reg);
    return (*this);
}

Program& Program::clbind(int_op reg) {
    /*  Inserts clbing instuction.
     */
    addr_ptr = cg::bytecode::clbind(addr_ptr, reg);
    return (*this);
}

Program& Program::closure(string fn, int_op reg) {
    /*  Inserts closure instuction.
     */
    addr_ptr = cg::bytecode::closure(addr_ptr, fn, reg);
    return (*this);
}

Program& Program::function(string fn, int_op reg) {
    /*  Inserts function instuction.
     */
    addr_ptr = cg::bytecode::function(addr_ptr, fn, reg);
    return (*this);
}

Program& Program::fcall(int_op clsr, int_op ret) {
    /*  Inserts fcall instruction to bytecode.
     */
    addr_ptr = cg::bytecode::fcall(addr_ptr, clsr, ret);
    return (*this);
}

Program& Program::frame(int_op a, int_op b) {
    /*  Inserts frame instruction to bytecode.
     */
    addr_ptr = cg::bytecode::frame(addr_ptr, a, b);
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
    addr_ptr = cg::bytecode::param(addr_ptr, a, b);
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
    addr_ptr = cg::bytecode::paref(addr_ptr, a, b);
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
    addr_ptr = cg::bytecode::arg(addr_ptr, a, b);
    return (*this);
}

Program& Program::call(string fn_name, int_op reg) {
    /*  Inserts call instruction.
     *  Byte offset is calculated automatically.
     */
    addr_ptr = cg::bytecode::call(addr_ptr, fn_name, reg);
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
    // save jump position if jump is not to byte
    if (is_absolute != JMP_TO_BYTE) {
        (is_absolute == JMP_ABSOLUTE ? branches_absolute : branches).push_back((addr_ptr+1));
    }

    addr_ptr = cg::bytecode::jump(addr_ptr, addr);
    return (*this);
}

Program& Program::branch(int_op regc, int addr_truth, enum JUMPTYPE absolute_truth, int addr_false, enum JUMPTYPE absolute_false) {
    /*  Inserts branch instruction.
     *  Byte offset is calculated automatically.
     */
    byte* jump_position_in_bytecode = addr_ptr;

    jump_position_in_bytecode += sizeof(byte); // for opcode
    jump_position_in_bytecode += sizeof(bool); // for at-register flag
    jump_position_in_bytecode += sizeof(int);  // for integer with register index
    // save jump position if jump is not to byte
    if (absolute_truth != JMP_TO_BYTE) {
        (absolute_truth == JMP_ABSOLUTE ? branches_absolute : branches).push_back(jump_position_in_bytecode);
    }

    jump_position_in_bytecode += sizeof(int);  // for integer with jump address
    // save jump position if jump is not to byte
    if (absolute_false != JMP_TO_BYTE) {
        (absolute_truth == JMP_ABSOLUTE ? branches_absolute : branches).push_back(jump_position_in_bytecode);
    }

    addr_ptr = cg::bytecode::branch(addr_ptr, regc, addr_truth, addr_false);
    return (*this);
}

Program& Program::tryframe() {
    /*  Inserts tryframe instruction.
     */
    addr_ptr = cg::bytecode::tryframe(addr_ptr);
    return (*this);
}

Program& Program::vmcatch(string type_name, string block_name) {
    /*  Inserts catch instruction.
     */
    addr_ptr = cg::bytecode::vmcatch(addr_ptr, type_name, block_name);
    return (*this);
}

Program& Program::pull(int_op regno) {
    /*  Inserts throw instuction.
     */
    addr_ptr = cg::bytecode::pull(addr_ptr, regno);
    return (*this);
}

Program& Program::vmtry(string block_name) {
    /*  Inserts try instruction.
     *  Byte offset is calculated automatically.
     */
    addr_ptr = cg::bytecode::vmtry(addr_ptr, block_name);
    return (*this);
}

Program& Program::vmthrow(int_op regno) {
    /*  Inserts throw instuction.
     */
    addr_ptr = cg::bytecode::vmthrow(addr_ptr, regno);
    return (*this);
}

Program& Program::leave() {
    /*  Inserts leave instruction.
     */
    addr_ptr = cg::bytecode::leave(addr_ptr);
    return (*this);
}

Program& Program::eximport(string module_name) {
    /*  Inserts eximport instruction.
     */
    addr_ptr = cg::bytecode::eximport(addr_ptr, module_name);
    return (*this);
}

Program& Program::excall(string fn_name, int_op reg) {
    /*  Inserts excall instruction.
     *  Byte offset is calculated automatically.
     */
    addr_ptr = cg::bytecode::excall(addr_ptr, fn_name, reg);
    return (*this);
}

Program& Program::end() {
    /*  Inserts end instruction.
     */
    addr_ptr = cg::bytecode::end(addr_ptr);
    return (*this);
}

Program& Program::halt() {
    /*  Inserts halt instruction.
     */
    addr_ptr = cg::bytecode::halt(addr_ptr);
    return (*this);
}
