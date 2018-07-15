/*
 *  Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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
#include <viua/program.h>
#include <viua/support/pointer.h>
using namespace std;


auto Program::opnop() -> Program& {
    addr_ptr = cg::bytecode::opnop(addr_ptr);
    return (*this);
}

auto Program::opizero(int_op const regno) -> Program& {
    addr_ptr = cg::bytecode::opizero(addr_ptr, regno);
    return (*this);
}

auto Program::opinteger(int_op const regno, int_op const i) -> Program& {
    addr_ptr = cg::bytecode::opinteger(addr_ptr, regno, i);
    return (*this);
}

auto Program::opiinc(int_op const regno) -> Program& {
    addr_ptr = cg::bytecode::opiinc(addr_ptr, regno);
    return (*this);
}

auto Program::opidec(int_op const regno) -> Program& {
    addr_ptr = cg::bytecode::opidec(addr_ptr, regno);
    return (*this);
}

auto Program::opfloat(int_op const regno, viua::internals::types::plain_float const f) -> Program& {
    addr_ptr = cg::bytecode::opfloat(addr_ptr, regno, f);
    return (*this);
}

auto Program::opitof(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::opitof(addr_ptr, a, b);
    return (*this);
}

auto Program::opftoi(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::opftoi(addr_ptr, a, b);
    return (*this);
}

auto Program::opstoi(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::opstoi(addr_ptr, a, b);
    return (*this);
}

auto Program::opstof(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::opstof(addr_ptr, a, b);
    return (*this);
}

auto Program::opadd(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opadd(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opsub(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opsub(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opmul(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opmul(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opdiv(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opdiv(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::oplt(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::oplt(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::oplte(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::oplte(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opgt(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opgt(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opgte(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opgte(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opeq(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opeq(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opstring(int_op const reg, std::string const s) -> Program& {
    addr_ptr = cg::bytecode::opstring(addr_ptr, reg, s);
    return (*this);
}

auto Program::optext(int_op const reg, std::string const s) -> Program& {
    addr_ptr = cg::bytecode::optext(addr_ptr, reg, s);
    return (*this);
}

auto Program::optext(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::optext(addr_ptr, a, b);
    return (*this);
}

auto Program::optexteq(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::optexteq(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::optextat(int_op const target, int_op const source, int_op const index) -> Program& {
    addr_ptr = cg::bytecode::optextat(addr_ptr, target, source, index);
    return (*this);
}
auto Program::optextsub(int_op const target, int_op const source, int_op const begin_index, int_op const end_index) -> Program& {
    addr_ptr = cg::bytecode::optextsub(
        addr_ptr, target, source, begin_index, end_index);
    return (*this);
}
auto Program::optextlength(int_op const target, int_op const source) -> Program& {
    addr_ptr = cg::bytecode::optextlength(addr_ptr, target, source);
    return (*this);
}
auto Program::optextcommonprefix(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::optextcommonprefix(addr_ptr, target, lhs, rhs);
    return (*this);
}
auto Program::optextcommonsuffix(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::optextcommonsuffix(addr_ptr, target, lhs, rhs);
    return (*this);
}
auto Program::optextconcat(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::optextconcat(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opvector(int_op const index, int_op const pack_start_index, int_op const pack_length) -> Program& {
    addr_ptr =
        cg::bytecode::opvector(addr_ptr, index, pack_start_index, pack_length);
    return (*this);
}

auto Program::opvinsert(int_op const vec, int_op const src, int_op const dst) -> Program& {
    addr_ptr = cg::bytecode::opvinsert(addr_ptr, vec, src, dst);
    return (*this);
}

auto Program::opvpush(int_op const vec, int_op const src) -> Program& {
    addr_ptr = cg::bytecode::opvpush(addr_ptr, vec, src);
    return (*this);
}

auto Program::opvpop(int_op const vec, int_op const dst, int_op const pos) -> Program& {
    addr_ptr = cg::bytecode::opvpop(addr_ptr, vec, dst, pos);
    return (*this);
}

auto Program::opvat(int_op const vec, int_op const dst, int_op const at) -> Program& {
    addr_ptr = cg::bytecode::opvat(addr_ptr, vec, dst, at);
    return (*this);
}

auto Program::opvlen(int_op const vec, int_op const reg) -> Program& {
    addr_ptr = cg::bytecode::opvlen(addr_ptr, vec, reg);
    return (*this);
}

auto Program::opnot(int_op const target, int_op const source) -> Program& {
    addr_ptr = cg::bytecode::opnot(addr_ptr, target, source);
    return (*this);
}

auto Program::opand(int_op const regr, int_op const rega, int_op const regb) -> Program& {
    addr_ptr = cg::bytecode::opand(addr_ptr, regr, rega, regb);
    return (*this);
}

auto Program::opor(int_op const regr, int_op const rega, int_op const regb) -> Program& {
    addr_ptr = cg::bytecode::opor(addr_ptr, regr, rega, regb);
    return (*this);
}

auto Program::opbits(int_op const target, int_op const count) -> Program& {
    addr_ptr = cg::bytecode::opbits(addr_ptr, target, count);
    return (*this);
}

auto Program::opbits(int_op const target, std::vector<uint8_t> const bit_string) -> Program& {
    addr_ptr = cg::bytecode::opbits(addr_ptr, target, bit_string);
    return (*this);
}

auto Program::opbitand(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opbitand(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opbitor(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opbitor(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opbitnot(int_op const target, int_op const lhs) -> Program& {
    addr_ptr = cg::bytecode::opbitnot(addr_ptr, target, lhs);
    return (*this);
}

auto Program::opbitxor(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opbitxor(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opbitat(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opbitat(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opbitset(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opbitset(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opbitset(int_op const target, int_op const lhs, bool const rhs) -> Program& {
    addr_ptr = cg::bytecode::opbitset(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opshl(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opshl(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opshr(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opshr(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opashl(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opashl(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opashr(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opashr(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::oprol(int_op const target, int_op const count) -> Program& {
    addr_ptr = cg::bytecode::oprol(addr_ptr, target, count);
    return (*this);
}

auto Program::opror(int_op const target, int_op const count) -> Program& {
    addr_ptr = cg::bytecode::opror(addr_ptr, target, count);
    return (*this);
}

auto Program::opwrapincrement(int_op const target) -> Program& {
    addr_ptr = cg::bytecode::opwrapincrement(addr_ptr, target);
    return (*this);
}

auto Program::opwrapdecrement(int_op const target) -> Program& {
    addr_ptr = cg::bytecode::opwrapdecrement(addr_ptr, target);
    return (*this);
}

auto Program::opwrapadd(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opwrapadd(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opwrapsub(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opwrapsub(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opwrapmul(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opwrapmul(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opwrapdiv(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opwrapdiv(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opcheckedsincrement(int_op const target) -> Program& {
    addr_ptr = cg::bytecode::opcheckedsincrement(addr_ptr, target);
    return (*this);
}

auto Program::opcheckedsdecrement(int_op const target) -> Program& {
    addr_ptr = cg::bytecode::opcheckedsdecrement(addr_ptr, target);
    return (*this);
}

auto Program::opcheckedsadd(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opcheckedsadd(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opcheckedssub(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opcheckedssub(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opcheckedsmul(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opcheckedsmul(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opcheckedsdiv(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opcheckedsdiv(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opcheckeduincrement(int_op const target) -> Program& {
    addr_ptr = cg::bytecode::opcheckeduincrement(addr_ptr, target);
    return (*this);
}

auto Program::opcheckedudecrement(int_op const target) -> Program& {
    addr_ptr = cg::bytecode::opcheckedudecrement(addr_ptr, target);
    return (*this);
}

auto Program::opcheckeduadd(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opcheckeduadd(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opcheckedusub(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opcheckedusub(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opcheckedumul(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opcheckedumul(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opcheckedudiv(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opcheckedudiv(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opsaturatingsincrement(int_op const target) -> Program& {
    addr_ptr = cg::bytecode::opsaturatingsincrement(addr_ptr, target);
    return (*this);
}

auto Program::opsaturatingsdecrement(int_op const target) -> Program& {
    addr_ptr = cg::bytecode::opsaturatingsdecrement(addr_ptr, target);
    return (*this);
}

auto Program::opsaturatingsadd(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opsaturatingsadd(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opsaturatingssub(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opsaturatingssub(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opsaturatingsmul(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opsaturatingsmul(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opsaturatingsdiv(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opsaturatingsdiv(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opsaturatinguincrement(int_op const target) -> Program& {
    addr_ptr = cg::bytecode::opsaturatinguincrement(addr_ptr, target);
    return (*this);
}

auto Program::opsaturatingudecrement(int_op const target) -> Program& {
    addr_ptr = cg::bytecode::opsaturatingudecrement(addr_ptr, target);
    return (*this);
}

auto Program::opsaturatinguadd(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opsaturatinguadd(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opsaturatingusub(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opsaturatingusub(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opsaturatingumul(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opsaturatingumul(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opsaturatingudiv(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opsaturatingudiv(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opmove(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::opmove(addr_ptr, a, b);
    return (*this);
}

auto Program::opcopy(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::opcopy(addr_ptr, a, b);
    return (*this);
}

auto Program::opptr(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::opptr(addr_ptr, a, b);
    return (*this);
}

auto Program::opptrlive(int_op const target, int_op const source) -> Program& {
    addr_ptr = cg::bytecode::opptrlive(addr_ptr, target, source);
    return (*this);
}

auto Program::opswap(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::opswap(addr_ptr, a, b);
    return (*this);
}

auto Program::opdelete(int_op const reg) -> Program& {
    addr_ptr = cg::bytecode::opdelete(addr_ptr, reg);
    return (*this);
}

auto Program::opisnull(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::opisnull(addr_ptr, a, b);
    return (*this);
}

auto Program::opprint(int_op const reg) -> Program& {
    addr_ptr = cg::bytecode::opprint(addr_ptr, reg);
    return (*this);
}

auto Program::opecho(int_op const reg) -> Program& {
    addr_ptr = cg::bytecode::opecho(addr_ptr, reg);
    return (*this);
}

auto Program::opcapture(int_op const target_closure, int_op const target_register, int_op const source_register) -> Program& {
    addr_ptr = cg::bytecode::opcapture(
        addr_ptr, target_closure, target_register, source_register);
    return (*this);
}

auto Program::opcapturecopy(int_op const target_closure, int_op const target_register, int_op const source_register) -> Program& {
    addr_ptr = cg::bytecode::opcapturecopy(
        addr_ptr, target_closure, target_register, source_register);
    return (*this);
}

auto Program::opcapturemove(int_op const target_closure, int_op const target_register, int_op const source_register) -> Program& {
    addr_ptr = cg::bytecode::opcapturemove(
        addr_ptr, target_closure, target_register, source_register);
    return (*this);
}

auto Program::opclosure(int_op const reg, std::string const& fn) -> Program& {
    addr_ptr = cg::bytecode::opclosure(addr_ptr, reg, fn);
    return (*this);
}

auto Program::opfunction(int_op const reg, std::string const& fn) -> Program& {
    addr_ptr = cg::bytecode::opfunction(addr_ptr, reg, fn);
    return (*this);
}

auto Program::opframe(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::opframe(addr_ptr, a, b);
    return (*this);
}

auto Program::opparam(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::opparam(addr_ptr, a, b);
    return (*this);
}

auto Program::oppamv(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::oppamv(addr_ptr, a, b);
    return (*this);
}

auto Program::oparg(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::oparg(addr_ptr, a, b);
    return (*this);
}

auto Program::opargc(int_op const a) -> Program& {
    addr_ptr = cg::bytecode::opargc(addr_ptr, a);
    return (*this);
}

auto Program::opallocate_registers(int_op const a) -> Program& {
    addr_ptr = cg::bytecode::opallocate_registers(addr_ptr, a);
    return (*this);
}

auto Program::opcall(int_op const reg, std::string const& fn_name) -> Program& {
    addr_ptr = cg::bytecode::opcall(addr_ptr, reg, fn_name);
    return (*this);
}

auto Program::opcall(int_op const reg, int_op const fn) -> Program& {
    addr_ptr = cg::bytecode::opcall(addr_ptr, reg, fn);
    return (*this);
}

auto Program::optailcall(std::string const& fn_name) -> Program& {
    addr_ptr = cg::bytecode::optailcall(addr_ptr, fn_name);
    return (*this);
}

auto Program::optailcall(int_op const fn) -> Program& {
    addr_ptr = cg::bytecode::optailcall(addr_ptr, fn);
    return (*this);
}

auto Program::opdefer(std::string const& fn_name) -> Program& {
    addr_ptr = cg::bytecode::opdefer(addr_ptr, fn_name);
    return (*this);
}

auto Program::opdefer(int_op const fn) -> Program& {
    addr_ptr = cg::bytecode::opdefer(addr_ptr, fn);
    return (*this);
}

auto Program::opprocess(int_op const ref, std::string const& fn_name) -> Program& {
    addr_ptr = cg::bytecode::opprocess(addr_ptr, ref, fn_name);
    return (*this);
}

auto Program::opprocess(int_op const reg, int_op const fn) -> Program& {
    addr_ptr = cg::bytecode::opprocess(addr_ptr, reg, fn);
    return (*this);
}

auto Program::opself(int_op const target) -> Program& {
    addr_ptr = cg::bytecode::opself(addr_ptr, target);
    return (*this);
}

auto Program::opjoin(int_op const target, int_op const source, timeout_op const timeout) -> Program& {
    addr_ptr = cg::bytecode::opjoin(addr_ptr, target, source, timeout);
    return (*this);
}

auto Program::opsend(int_op const target, int_op const source) -> Program& {
    addr_ptr = cg::bytecode::opsend(addr_ptr, target, source);
    return (*this);
}

auto Program::opreceive(int_op const ref, timeout_op const timeout) -> Program& {
    addr_ptr = cg::bytecode::opreceive(addr_ptr, ref, timeout);
    return (*this);
}

auto Program::opwatchdog(std::string const& fn_name) -> Program& {
    addr_ptr = cg::bytecode::opwatchdog(addr_ptr, fn_name);
    return (*this);
}

auto Program::opjump(viua::internals::types::bytecode_size const addr, enum JUMPTYPE const is_absolute) -> Program& {
    if (is_absolute != JMP_TO_BYTE) {
        branches.push_back((addr_ptr + 1));
    }

    addr_ptr = cg::bytecode::opjump(addr_ptr, addr);
    return (*this);
}

auto Program::opif(int_op const regc, viua::internals::types::bytecode_size const addr_truth, enum JUMPTYPE const absolute_truth, viua::internals::types::bytecode_size const addr_false, enum JUMPTYPE const absolute_false) -> Program& {
    auto jump_position_in_bytecode = addr_ptr;

    jump_position_in_bytecode +=
        sizeof(viua::internals::types::byte);  // for opcode
    jump_position_in_bytecode +=
        sizeof(viua::internals::types::byte);  // for operand-type marker
    jump_position_in_bytecode +=
        sizeof(viua::internals::Register_sets);  // for rs-type marker
    jump_position_in_bytecode += sizeof(viua::internals::types::register_index);

    if (absolute_truth != JMP_TO_BYTE) {
        branches.push_back(jump_position_in_bytecode);
    }
    jump_position_in_bytecode += sizeof(viua::internals::types::bytecode_size);

    if (absolute_false != JMP_TO_BYTE) {
        branches.push_back(jump_position_in_bytecode);
    }

    addr_ptr = cg::bytecode::opif(addr_ptr, regc, addr_truth, addr_false);
    return (*this);
}

auto Program::optry() -> Program& {
    addr_ptr = cg::bytecode::optry(addr_ptr);
    return (*this);
}

auto Program::opcatch(std::string const type_name, std::string const block_name) -> Program& {
    addr_ptr = cg::bytecode::opcatch(addr_ptr, type_name, block_name);
    return (*this);
}

auto Program::opdraw(int_op const regno) -> Program& {
    addr_ptr = cg::bytecode::opdraw(addr_ptr, regno);
    return (*this);
}

auto Program::openter(std::string const block_name) -> Program& {
    addr_ptr = cg::bytecode::openter(addr_ptr, block_name);
    return (*this);
}

auto Program::opthrow(int_op const regno) -> Program& {
    addr_ptr = cg::bytecode::opthrow(addr_ptr, regno);
    return (*this);
}

auto Program::opleave() -> Program& {
    addr_ptr = cg::bytecode::opleave(addr_ptr);
    return (*this);
}

auto Program::opimport(std::string const module_name) -> Program& {
    addr_ptr = cg::bytecode::opimport(addr_ptr, module_name);
    return (*this);
}

auto Program::opatom(int_op const reg, std::string const& s) -> Program& {
    addr_ptr = cg::bytecode::opatom(addr_ptr, reg, s);
    return (*this);
}

auto Program::opatomeq(int_op const target, int_op const lhs, int_op const rhs) -> Program& {
    addr_ptr = cg::bytecode::opatomeq(addr_ptr, target, lhs, rhs);
    return (*this);
}

auto Program::opstruct(int_op const regno) -> Program& {
    addr_ptr = cg::bytecode::opstruct(addr_ptr, regno);
    return (*this);
}

auto Program::opstructinsert(int_op const target, int_op const key, int_op const source) -> Program& {
    addr_ptr = cg::bytecode::opstructinsert(addr_ptr, target, key, source);
    return (*this);
}

auto Program::opstructremove(int_op const target, int_op const key, int_op const source) -> Program& {
    addr_ptr = cg::bytecode::opstructremove(addr_ptr, target, key, source);
    return (*this);
}

auto Program::opstructkeys(int_op const a, int_op const b) -> Program& {
    addr_ptr = cg::bytecode::opstructkeys(addr_ptr, a, b);
    return (*this);
}

auto Program::opreturn() -> Program& {
    addr_ptr = cg::bytecode::opreturn(addr_ptr);
    return (*this);
}

auto Program::ophalt() -> Program& {
    /*  Inserts halt instruction.
     */
    addr_ptr = cg::bytecode::ophalt(addr_ptr);
    return (*this);
}
