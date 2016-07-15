/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <viua/bytecode/opcodes.h>
#include <viua/support/pointer.h>
#include <viua/program.h>
using namespace std;


Program& Program::opnop() {
    /*  Inserts nop instuction.
     */
    addr_ptr = cg::bytecode::opnop(addr_ptr);
    return (*this);
}

Program& Program::opizero(int_op regno) {
    /*  Inserts izero instuction.
     */
    addr_ptr = cg::bytecode::opizero(addr_ptr, regno);
    return (*this);
}

Program& Program::opistore(int_op regno, int_op i) {
    /*  Inserts istore instruction to bytecode.
     *
     *  :params:
     *
     *  regno:int - register number
     *  i:int     - value to store
     */
    addr_ptr = cg::bytecode::opistore(addr_ptr, regno, i);
    return (*this);
}

Program& Program::opiadd(int_op rega, int_op regb, int_op regr) {
    /*  Inserts iadd instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opiadd(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opisub(int_op rega, int_op regb, int_op regr) {
    /*  Inserts isub instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opisub(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opimul(int_op rega, int_op regb, int_op regr) {
    /*  Inserts imul instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opimul(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opidiv(int_op rega, int_op regb, int_op regr) {
    /*  Inserts idiv instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opidiv(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opiinc(int_op regno) {
    /*  Inserts iinc instuction.
     */
    addr_ptr = cg::bytecode::opiinc(addr_ptr, regno);
    return (*this);
}

Program& Program::opidec(int_op regno) {
    /*  Inserts idec instuction.
     */
    addr_ptr = cg::bytecode::opidec(addr_ptr, regno);
    return (*this);
}

Program& Program::opilt(int_op rega, int_op regb, int_op regr) {
    /*  Inserts ilt instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opilt(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opilte(int_op rega, int_op regb, int_op regr) {
    /*  Inserts ilte instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opilte(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opigt(int_op rega, int_op regb, int_op regr) {
    /*  Inserts igt instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opigt(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opigte(int_op rega, int_op regb, int_op regr) {
    /*  Inserts igte instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opigte(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opieq(int_op rega, int_op regb, int_op regr) {
    /*  Inserts ieq instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opieq(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opfstore(int_op regno, float f) {
    /*  Inserts fstore instruction to bytecode.
     *
     *  :params:
     *
     *  regno - register number
     *  f     - value to store
     */
    addr_ptr = cg::bytecode::opfstore(addr_ptr, regno, f);
    return (*this);
}

Program& Program::opfadd(int_op rega, int_op regb, int_op regr) {
    /*  Inserts fadd instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opfadd(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opfsub(int_op rega, int_op regb, int_op regr) {
    /*  Inserts fsub instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opfsub(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opfmul(int_op rega, int_op regb, int_op regr) {
    /*  Inserts fmul instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opfmul(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opfdiv(int_op rega, int_op regb, int_op regr) {
    /*  Inserts fdiv instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opfdiv(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opflt(int_op rega, int_op regb, int_op regr) {
    /*  Inserts flt instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opflt(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opflte(int_op rega, int_op regb, int_op regr) {
    /*  Inserts flte instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the result
     */
    addr_ptr = cg::bytecode::opflte(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opfgt(int_op rega, int_op regb, int_op regr) {
    /*  Inserts fgt instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the resugt
     */
    addr_ptr = cg::bytecode::opfgt(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opfgte(int_op rega, int_op regb, int_op regr) {
    /*  Inserts fgte instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the resugt
     */
    addr_ptr = cg::bytecode::opfgte(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opfeq(int_op rega, int_op regb, int_op regr) {
    /*  Inserts feq instruction to bytecode.
     *
     *  :params:
     *
     *  rega    - register index of first operand
     *  regb    - register index of second operand
     *  regr    - register index in which to store the resugt
     */
    addr_ptr = cg::bytecode::opfeq(addr_ptr, rega, regb, regr);
    return (*this);
}

Program& Program::opbstore(int_op regno, byte_op b) {
    /*  Inserts bstore instruction to bytecode.
     *
     *  :params:
     *
     *  regno - register number
     *  b     - value to store
     */
    addr_ptr = cg::bytecode::opbstore(addr_ptr, regno, b);
    return (*this);
}

Program& Program::opitof(int_op a, int_op b) {
    /*  Inserts itof instruction to bytecode.
     */
    addr_ptr = cg::bytecode::opitof(addr_ptr, a, b);
    return (*this);
}

Program& Program::opftoi(int_op a, int_op b) {
    /*  Inserts ftoi instruction to bytecode.
     */
    addr_ptr = cg::bytecode::opftoi(addr_ptr, a, b);
    return (*this);
}

Program& Program::opstoi(int_op a, int_op b) {
    /*  Inserts stoi instruction to bytecode.
     */
    addr_ptr = cg::bytecode::opstoi(addr_ptr, a, b);
    return (*this);
}

Program& Program::opstof(int_op a, int_op b) {
    /*  Inserts stof instruction to bytecode.
     */
    addr_ptr = cg::bytecode::opstof(addr_ptr, a, b);
    return (*this);
}

Program& Program::opstrstore(int_op reg, string s) {
    /*  Inserts strstore instruction.
     */
    addr_ptr = cg::bytecode::opstrstore(addr_ptr, reg, s);
    return (*this);
}

Program& Program::opvec(int_op index, int_op pack_start_index, int_op pack_length) {
    /** Inserts vec instruction.
     */
    addr_ptr = cg::bytecode::opvec(addr_ptr, index, pack_start_index, pack_length);
    return (*this);
}

Program& Program::opvinsert(int_op vec, int_op src, int_op dst) {
    /** Inserts vinsert instruction.
     */
    addr_ptr = cg::bytecode::opvinsert(addr_ptr, vec, src, dst);
    return (*this);
}

Program& Program::opvpush(int_op vec, int_op src) {
    /** Inserts vpush instruction.
     */
    addr_ptr = cg::bytecode::opvpush(addr_ptr, vec, src);
    return (*this);
}

Program& Program::opvpop(int_op vec, int_op dst, int_op pos) {
    /** Inserts vpop instruction.
     */
    addr_ptr = cg::bytecode::opvpop(addr_ptr, vec, dst, pos);
    return (*this);
}

Program& Program::opvat(int_op vec, int_op dst, int_op at) {
    /** Inserts vat instruction.
     */
    addr_ptr = cg::bytecode::opvat(addr_ptr, vec, dst, at);
    return (*this);
}

Program& Program::opvlen(int_op vec, int_op reg) {
    /** Inserts vlen instruction.
     */
    addr_ptr = cg::bytecode::opvlen(addr_ptr, vec, reg);
    return (*this);
}

Program& Program::opnot(int_op reg) {
    /*  Inserts not instuction.
     */
    addr_ptr = cg::bytecode::opnot(addr_ptr, reg);
    return (*this);
}

Program& Program::opand(int_op regr, int_op rega, int_op regb) {
    /*  Inserts and instruction to bytecode.
     *
     *  :params:
     *
     *  regr   - register index in which to store the result
     *  rega   - register index of first operand
     *  regb   - register index of second operand
     */
    addr_ptr = cg::bytecode::opand(addr_ptr, regr, rega, regb);
    return (*this);
}

Program& Program::opor(int_op regr, int_op rega, int_op regb) {
    /*  Inserts or instruction to bytecode.
     *
     *  :params:
     *
     *  regr   - register index in which to store the result
     *  rega   - register index of first operand
     *  regb   - register index of second operand
     */
    addr_ptr = cg::bytecode::opor(addr_ptr, regr, rega, regb);
    return (*this);
}

Program& Program::opmove(int_op a, int_op b) {
    /*  Inserts move instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number (move from...)
     *  b - register number (move to...)
     */
    addr_ptr = cg::bytecode::opmove(addr_ptr, a, b);
    return (*this);
}

Program& Program::opcopy(int_op a, int_op b) {
    /*  Inserts copy instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number (copy from...)
     *  b - register number (copy to...)
     */
    addr_ptr = cg::bytecode::opcopy(addr_ptr, a, b);
    return (*this);
}

Program& Program::opptr(int_op a, int_op b) {
    addr_ptr = cg::bytecode::opptr(addr_ptr, a, b);
    return (*this);
}

Program& Program::opswap(int_op a, int_op b) {
    /*  Inserts swap instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number
     *  b - register number
     */
    addr_ptr = cg::bytecode::opswap(addr_ptr, a, b);
    return (*this);
}

Program& Program::opdelete(int_op reg) {
    /*  Inserts delete instuction.
     */
    addr_ptr = cg::bytecode::opdelete(addr_ptr, reg);
    return (*this);
}

Program& Program::opempty(int_op reg) {
    /*  Inserts empty instuction.
     */
    addr_ptr = cg::bytecode::opempty(addr_ptr, reg);
    return (*this);
}

Program& Program::opisnull(int_op a, int_op b) {
    /*  Inserts isnull instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number
     *  b - register number
     */
    addr_ptr = cg::bytecode::opisnull(addr_ptr, a, b);
    return (*this);
}

Program& Program::opress(string a) {
    /*  Inserts ress instruction to bytecode.
     *
     *  :params:
     *
     *  a - register set ID
     */
    addr_ptr = cg::bytecode::opress(addr_ptr, a);
    return (*this);
}

Program& Program::optmpri(int_op reg) {
    /*  Inserts tmpri instuction.
     */
    addr_ptr = cg::bytecode::optmpri(addr_ptr, reg);
    return (*this);
}

Program& Program::optmpro(int_op reg) {
    /*  Inserts tmpro instuction.
     */
    addr_ptr = cg::bytecode::optmpro(addr_ptr, reg);
    return (*this);
}

Program& Program::opprint(int_op reg) {
    /*  Inserts print instuction.
     */
    addr_ptr = cg::bytecode::opprint(addr_ptr, reg);
    return (*this);
}

Program& Program::opecho(int_op reg) {
    /*  Inserts echo instuction.
     */
    addr_ptr = cg::bytecode::opecho(addr_ptr, reg);
    return (*this);
}

Program& Program::openclose(int_op target_closure, int_op target_register, int_op source_register) {
    /*  Inserts clbing instuction.
     */
    addr_ptr = cg::bytecode::openclose(addr_ptr, target_closure, target_register, source_register);
    return (*this);
}

Program& Program::openclosecopy(int_op target_closure, int_op target_register, int_op source_register) {
    /*  Inserts openclosecopy instuction.
     */
    addr_ptr = cg::bytecode::openclosecopy(addr_ptr, target_closure, target_register, source_register);
    return (*this);
}

Program& Program::openclosemove(int_op target_closure, int_op target_register, int_op source_register) {
    /*  Inserts openclosemove instuction.
     */
    addr_ptr = cg::bytecode::openclosemove(addr_ptr, target_closure, target_register, source_register);
    return (*this);
}

Program& Program::opclosure(int_op reg, const string& fn) {
    /*  Inserts closure instuction.
     */
    addr_ptr = cg::bytecode::opclosure(addr_ptr, reg, fn);
    return (*this);
}

Program& Program::opfunction(int_op reg, const string& fn) {
    /*  Inserts function instuction.
     */
    addr_ptr = cg::bytecode::opfunction(addr_ptr, reg, fn);
    return (*this);
}

Program& Program::opfcall(int_op clsr, int_op ret) {
    /*  Inserts fcall instruction to bytecode.
     */
    addr_ptr = cg::bytecode::opfcall(addr_ptr, clsr, ret);
    return (*this);
}

Program& Program::opframe(int_op a, int_op b) {
    /*  Inserts frame instruction to bytecode.
     */
    addr_ptr = cg::bytecode::opframe(addr_ptr, a, b);
    return (*this);
}

Program& Program::opparam(int_op a, int_op b) {
    /*  Inserts param instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number
     *  b - register number
     */
    addr_ptr = cg::bytecode::opparam(addr_ptr, a, b);
    return (*this);
}

Program& Program::oppamv(int_op a, int_op b) {
    /*  Inserts pamv instruction to bytecode.
     *
     *  :params:
     *
     *  a - register number
     *  b - register number
     */
    addr_ptr = cg::bytecode::oppamv(addr_ptr, a, b);
    return (*this);
}

Program& Program::oparg(int_op a, int_op b) {
    /*  Inserts arg instruction to bytecode.
     *
     *  :params:
     *
     *  a - argument number
     *  b - register number
     */
    addr_ptr = cg::bytecode::oparg(addr_ptr, a, b);
    return (*this);
}

Program& Program::opargc(int_op a) {
    /*  Inserts argc instruction to bytecode.
     *
     *  :params:
     *
     *  a - target register
     */
    addr_ptr = cg::bytecode::opargc(addr_ptr, a);
    return (*this);
}

Program& Program::opcall(int_op reg, const string& fn_name) {
    /*  Inserts call instruction.
     *  Byte offset is calculated automatically.
     */
    addr_ptr = cg::bytecode::opcall(addr_ptr, reg, fn_name);
    return (*this);
}

Program& Program::optailcall(const string& fn_name) {
    /*  Inserts tailcall instruction.
     *  Byte offset is calculated automatically.
     */
    addr_ptr = cg::bytecode::optailcall(addr_ptr, fn_name);
    return (*this);
}

Program& Program::opprocess(int_op ref, const string& fn_name) {
    addr_ptr = cg::bytecode::opprocess(addr_ptr, ref, fn_name);
    return (*this);
}

Program& Program::opjoin(int_op target, int_op source) {
    addr_ptr = cg::bytecode::opjoin(addr_ptr, target, source);
    return (*this);
}

Program& Program::opreceive(int_op ref) {
    addr_ptr = cg::bytecode::opreceive(addr_ptr, ref);
    return (*this);
}

Program& Program::opwatchdog(const string& fn_name) {
    addr_ptr = cg::bytecode::opwatchdog(addr_ptr, fn_name);
    return (*this);
}

Program& Program::opjump(uint64_t addr, enum JUMPTYPE is_absolute) {
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

    addr_ptr = cg::bytecode::opjump(addr_ptr, addr);
    return (*this);
}

Program& Program::opbranch(int_op regc, uint64_t addr_truth, enum JUMPTYPE absolute_truth, uint64_t addr_false, enum JUMPTYPE absolute_false) {
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

    jump_position_in_bytecode += sizeof(uint64_t);  // for integer with jump address
    // save jump position if jump is not to byte
    if (absolute_false != JMP_TO_BYTE) {
        (absolute_truth == JMP_ABSOLUTE ? branches_absolute : branches).push_back(jump_position_in_bytecode);
    }

    addr_ptr = cg::bytecode::opbranch(addr_ptr, regc, addr_truth, addr_false);
    return (*this);
}

Program& Program::optry() {
    /*  Inserts try instruction.
     */
    addr_ptr = cg::bytecode::optry(addr_ptr);
    return (*this);
}

Program& Program::opcatch(string type_name, string block_name) {
    /*  Inserts catch instruction.
     */
    addr_ptr = cg::bytecode::opcatch(addr_ptr, type_name, block_name);
    return (*this);
}

Program& Program::oppull(int_op regno) {
    /*  Inserts throw instuction.
     */
    addr_ptr = cg::bytecode::oppull(addr_ptr, regno);
    return (*this);
}

Program& Program::openter(string block_name) {
    /*  Inserts enter instruction.
     *  Byte offset is calculated automatically.
     */
    addr_ptr = cg::bytecode::openter(addr_ptr, block_name);
    return (*this);
}

Program& Program::opthrow(int_op regno) {
    /*  Inserts throw instuction.
     */
    addr_ptr = cg::bytecode::opthrow(addr_ptr, regno);
    return (*this);
}

Program& Program::opleave() {
    /*  Inserts leave instruction.
     */
    addr_ptr = cg::bytecode::opleave(addr_ptr);
    return (*this);
}

Program& Program::opimport(string module_name) {
    /*  Inserts import instruction.
     */
    addr_ptr = cg::bytecode::opimport(addr_ptr, module_name);
    return (*this);
}

Program& Program::oplink(string module_name) {
    /*  Inserts link instruction.
     */
    addr_ptr = cg::bytecode::oplink(addr_ptr, module_name);
    return (*this);
}

Program& Program::opclass(int_op reg, const string& class_name) {
    /*  Inserts class instuction.
     */
    addr_ptr = cg::bytecode::opclass(addr_ptr, reg, class_name);
    return (*this);
}

Program& Program::opderive(int_op reg, const string& base_class_name) {
    /*  Inserts derive instuction.
     */
    addr_ptr = cg::bytecode::opderive(addr_ptr, reg, base_class_name);
    return (*this);
}

Program& Program::opattach(int_op reg, const string& function_name, const string& method_name) {
    /*  Inserts attach instuction.
     */
    addr_ptr = cg::bytecode::opattach(addr_ptr, reg, function_name, method_name);
    return (*this);
}

Program& Program::opregister(int_op reg) {
    /*  Inserts register instuction.
     */
    addr_ptr = cg::bytecode::opregister(addr_ptr, reg);
    return (*this);
}

Program& Program::opnew(int_op reg, const string& class_name) {
    /*  Inserts new instuction.
     */
    addr_ptr = cg::bytecode::opnew(addr_ptr, reg, class_name);
    return (*this);
}

Program& Program::opmsg(int_op reg, const string& method_name) {
    /*  Inserts msg instuction.
     */
    addr_ptr = cg::bytecode::opmsg(addr_ptr, reg, method_name);
    return (*this);
}

Program& Program::opinsert(int_op target, int_op key, int_op source) {
    addr_ptr = cg::bytecode::opinsert(addr_ptr, target, key, source);
    return (*this);
}

Program& Program::opremove(int_op target, int_op key, int_op source) {
    addr_ptr = cg::bytecode::opremove(addr_ptr, target, key, source);
    return (*this);
}

Program& Program::opreturn() {
    addr_ptr = cg::bytecode::opreturn(addr_ptr);
    return (*this);
}

Program& Program::ophalt() {
    /*  Inserts halt instruction.
     */
    addr_ptr = cg::bytecode::ophalt(addr_ptr);
    return (*this);
}
