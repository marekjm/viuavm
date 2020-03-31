/*
 *  Copyright (C) 2015-2020 Marek Marecki
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

#include <cassert>
#include <viua/bytecode/opcodes.h>
#include <viua/program.h>
#include <viua/support/pointer.h>


int_op::int_op()
        : type(Integer_operand_type::PLAIN)
        , rs_type(viua::bytecode::codec::Register_set::Local)
        , value(0)
{}
int_op::int_op(Integer_operand_type t, viua::bytecode::codec::plain_int_type n)
        : type(t), rs_type(viua::bytecode::codec::Register_set::Local), value(n)
{}
int_op::int_op(Integer_operand_type t,
               viua::bytecode::codec::Register_set rst,
               viua::bytecode::codec::plain_int_type n)
        : type(t), rs_type(rst), value(n)
{}
int_op::int_op(viua::bytecode::codec::plain_int_type n)
        : type(Integer_operand_type::PLAIN)
        , rs_type(viua::bytecode::codec::Register_set::Local)
        , value(n)
{}

timeout_op::timeout_op() : type(Integer_operand_type::PLAIN), value(0) {}
timeout_op::timeout_op(Integer_operand_type t,
                       viua::bytecode::codec::timeout_type n)
        : type(t), value(n)
{}
timeout_op::timeout_op(viua::bytecode::codec::timeout_type n)
        : type(Integer_operand_type::PLAIN), value(n)
{}


auto Program::opnop() -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::NOP);
    return (*this);
}

static auto ra_of_intop(int_op const x, std::optional<Integer_operand_type> const alt_type = std::nullopt)
    -> viua::bytecode::codec::Register_access
{
    using namespace viua::bytecode::codec;

    Access_specifier access = Access_specifier::Direct;
    auto const xt = (alt_type.has_value() ? *alt_type : x.type);
    switch (xt) {
    case Integer_operand_type::INDEX:
        access = Access_specifier::Direct;
        break;
    case Integer_operand_type::REGISTER_REFERENCE:
        access = Access_specifier::Register_indirect;
        break;
    case Integer_operand_type::POINTER_DEREFERENCE:
        access = Access_specifier::Pointer_dereference;
        break;
    case Integer_operand_type::PLAIN:
    case Integer_operand_type::VOID:
    default:
        assert(0);
    }

    return Register_access{static_cast<Register_set>(x.rs_type),
                           static_cast<register_index_type>(x.value),
                           access};
}

auto Program::opizero(int_op const regno) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::IZERO);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(regno));
    return (*this);
}

auto Program::opinteger(int_op const regno, int_op const i) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::INTEGER);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(regno));
    addr_ptr = encoder.encode_i32(addr_ptr, i.value);
    return (*this);
}

auto Program::opiinc(int_op const regno) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::IINC);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(regno));
    return (*this);
}

auto Program::opidec(int_op const regno) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::IDEC);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(regno));
    return (*this);
}

auto Program::opfloat(int_op const regno,
                      viua::bytecode::codec::plain_float_type const f) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::FLOAT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(regno));
    addr_ptr = encoder.encode_f64(addr_ptr, f);
    return (*this);
}

auto Program::opitof(int_op const a, int_op const b) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::ITOF);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(a));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(b));
    return (*this);
}

auto Program::opftoi(int_op const a, int_op const b) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::FTOI);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(a));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(b));
    return (*this);
}

auto Program::opstoi(int_op const a, int_op const b) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::STOI);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(a));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(b));
    return (*this);
}

auto Program::opstof(int_op const a, int_op const b) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::STOF);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(a));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(b));
    return (*this);
}

auto Program::opadd(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::ADD);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opsub(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SUB);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opmul(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::MUL);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opdiv(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::DIV);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::oplt(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::LT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::oplte(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::LTE);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opgt(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::GT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opgte(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::GTE);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opeq(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::EQ);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opstring(int_op const reg, std::string const s) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::STRING);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(reg));
    addr_ptr = encoder.encode_string(addr_ptr, s.substr(1, s.size() - 2));
    return (*this);
}

auto Program::optext(int_op const reg, std::string const s) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::TEXT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(reg));
    addr_ptr = encoder.encode_string(addr_ptr, s.substr(1, s.size() - 2));
    return (*this);
}

auto Program::optext(int_op const a, int_op const b) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::TEXT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(a));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(b));
    return (*this);
}

auto Program::optexteq(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::TEXTEQ);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::optextat(int_op const target,
                       int_op const source,
                       int_op const index) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::TEXTAT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(index));
    return (*this);
}
auto Program::optextsub(int_op const target,
                        int_op const source,
                        int_op const begin_index,
                        int_op const end_index) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::TEXTSUB);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(begin_index));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(end_index));
    return (*this);
}
auto Program::optextlength(int_op const target, int_op const source) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::TEXTLENGTH);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    return (*this);
}
auto Program::optextcommonprefix(int_op const target,
                                 int_op const lhs,
                                 int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::TEXTCOMMONPREFIX);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}
auto Program::optextcommonsuffix(int_op const target,
                                 int_op const lhs,
                                 int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::TEXTCOMMONSUFFIX);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}
auto Program::optextconcat(int_op const target,
                           int_op const lhs,
                           int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::TEXTCONCAT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opvector(int_op const target,
                       int_op const pack_start_index,
                       int_op const pack_length) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::VECTOR);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(pack_start_index, Integer_operand_type::INDEX));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(pack_length, Integer_operand_type::INDEX));
    return (*this);
}

auto Program::opvinsert(int_op const vec, int_op const src, int_op const dst)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::VINSERT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(vec));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(src));
    if (dst.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(dst));
    }
    return (*this);
}

auto Program::opvpush(int_op const vec, int_op const src) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::VPUSH);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(vec));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(src));
    return (*this);
}

auto Program::opvpop(int_op const dst, int_op const vec, int_op const pos)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::VPOP);
    if (dst.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(dst));
    }
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(vec));
    if (pos.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(pos));
    }
    return (*this);
}

auto Program::opvat(int_op const vec, int_op const dst, int_op const at)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::VAT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(vec));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(dst));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(at));
    return (*this);
}

auto Program::opvlen(int_op const vec, int_op const reg) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::VLEN);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(vec));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(reg));
    return (*this);
}

auto Program::opnot(int_op const target, int_op const source) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::NOT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    return (*this);
}

auto Program::opand(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::AND);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opor(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::OR);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opbits_of_integer(int_op const a, int_op const b) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::BITS_OF_INTEGER);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(a));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(b));
    return (*this);
}

auto Program::opinteger_of_bits(int_op const a, int_op const b) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::INTEGER_OF_BITS);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(a));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(b));
    return (*this);
}

auto Program::opbits(int_op const target, int_op const count) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::BITS);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(count));
    return (*this);
}

auto Program::opbits(int_op const target, std::vector<uint8_t> const bit_string)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::BITS);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_bits_string(addr_ptr, bit_string);
    return (*this);
}

auto Program::opbitand(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::BITAND);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opbitor(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::BITOR);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opbitnot(int_op const target, int_op const lhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::BITNOT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    return (*this);
}

auto Program::opbitxor(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::BITXOR);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opbitat(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::BITAT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opbitset(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::BITSET);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opbitset(int_op const target, int_op const lhs, bool const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::BITSET);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_bool(addr_ptr, rhs);
    return (*this);
}

auto Program::opshl(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SHL);
    if (target.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    }
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opshr(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SHR);
    if (target.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    }
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opashl(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::ASHL);
    if (target.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    }
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opashr(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::ASHR);
    if (target.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    }
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::oprol(int_op const target, int_op const count) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::ROL);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(count));
    return (*this);
}

auto Program::opror(int_op const target, int_op const count) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::ROR);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(count));
    return (*this);
}

auto Program::opwrapincrement(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::WRAPINCREMENT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    return (*this);
}

auto Program::opwrapdecrement(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::WRAPDECREMENT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    return (*this);
}

auto Program::opwrapadd(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::WRAPADD);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opwrapsub(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::WRAPSUB);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opwrapmul(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::WRAPMUL);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opwrapdiv(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::WRAPDIV);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opcheckedsincrement(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CHECKEDSINCREMENT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    return (*this);
}

auto Program::opcheckedsdecrement(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CHECKEDSDECREMENT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    return (*this);
}

auto Program::opcheckedsadd(int_op const target,
                            int_op const lhs,
                            int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CHECKEDSADD);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opcheckedssub(int_op const target,
                            int_op const lhs,
                            int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CHECKEDSSUB);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opcheckedsmul(int_op const target,
                            int_op const lhs,
                            int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CHECKEDSMUL);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opcheckedsdiv(int_op const target,
                            int_op const lhs,
                            int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CHECKEDSDIV);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opcheckeduincrement(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CHECKEDUINCREMENT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    return (*this);
}

auto Program::opcheckedudecrement(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CHECKEDUDECREMENT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    return (*this);
}

auto Program::opcheckeduadd(int_op const target,
                            int_op const lhs,
                            int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CHECKEDUADD);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opcheckedusub(int_op const target,
                            int_op const lhs,
                            int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CHECKEDUSUB);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opcheckedumul(int_op const target,
                            int_op const lhs,
                            int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CHECKEDUMUL);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opcheckedudiv(int_op const target,
                            int_op const lhs,
                            int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CHECKEDUDIV);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opsaturatingsincrement(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SATURATINGSINCREMENT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    return (*this);
}

auto Program::opsaturatingsdecrement(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SATURATINGSDECREMENT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    return (*this);
}

auto Program::opsaturatingsadd(int_op const target,
                               int_op const lhs,
                               int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SATURATINGSADD);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opsaturatingssub(int_op const target,
                               int_op const lhs,
                               int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SATURATINGSSUB);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opsaturatingsmul(int_op const target,
                               int_op const lhs,
                               int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SATURATINGSMUL);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opsaturatingsdiv(int_op const target,
                               int_op const lhs,
                               int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SATURATINGSDIV);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opsaturatinguincrement(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SATURATINGUINCREMENT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    return (*this);
}

auto Program::opsaturatingudecrement(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SATURATINGUDECREMENT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    return (*this);
}

auto Program::opsaturatinguadd(int_op const target,
                               int_op const lhs,
                               int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SATURATINGUADD);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opsaturatingusub(int_op const target,
                               int_op const lhs,
                               int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SATURATINGUSUB);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opsaturatingumul(int_op const target,
                               int_op const lhs,
                               int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SATURATINGUMUL);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opsaturatingudiv(int_op const target,
                               int_op const lhs,
                               int_op const rhs) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SATURATINGUDIV);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opmove(int_op const dst, int_op const src) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::MOVE);
    if (dst.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(dst));
    }
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(src));
    return (*this);
}

auto Program::opcopy(int_op const dst, int_op const src) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::COPY);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(dst));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(src));
    return (*this);
}

auto Program::opptr(int_op const dst, int_op const src) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::PTR);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(dst));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(src));
    return (*this);
}

auto Program::opptrlive(int_op const target, int_op const source) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::PTRLIVE);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    return (*this);
}

auto Program::opswap(int_op const a, int_op const b) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SWAP);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(a));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(b));
    return (*this);
}

auto Program::opdelete(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::DELETE);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    return (*this);
}

auto Program::opisnull(int_op const dst, int_op const src) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::ISNULL);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(dst));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(src));
    return (*this);
}

auto Program::opprint(int_op const src) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::PRINT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(src));
    return (*this);
}

auto Program::opecho(int_op const src) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::ECHO);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(src));
    return (*this);
}

auto Program::opcapture(int_op const target_closure,
                        int_op const target_register,
                        int_op const source) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CAPTURE);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target_closure));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target_register, Integer_operand_type::INDEX));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    return (*this);
}

auto Program::opcapturecopy(int_op const target_closure,
                            int_op const target_register,
                            int_op const source) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CAPTURECOPY);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target_closure));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target_register, Integer_operand_type::REGISTER_REFERENCE));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    return (*this);
}

auto Program::opcapturemove(int_op const target_closure,
                            int_op const target_register,
                            int_op const source) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CAPTUREMOVE);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target_closure));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target_register, Integer_operand_type::REGISTER_REFERENCE));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    return (*this);
}

auto Program::opclosure(int_op const reg, std::string const& fn) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CLOSURE);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(reg));
    addr_ptr = encoder.encode_raw_string(addr_ptr, fn);
    return (*this);
}

auto Program::opfunction(int_op const reg, std::string const& fn) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::FUNCTION);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(reg));
    addr_ptr = encoder.encode_raw_string(addr_ptr, fn);
    return (*this);
}

auto Program::opframe(int_op const args) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::FRAME);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(args, Integer_operand_type::INDEX));
    return (*this);
}

auto Program::opallocate_registers(int_op const size) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::ALLOCATE_REGISTERS);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(size));
    return (*this);
}

auto Program::opcall(int_op const ret, std::string const& fn) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CALL);
    if (ret.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(ret));
    }
    addr_ptr = encoder.encode_raw_string(addr_ptr, fn);
    return (*this);
}

auto Program::opcall(int_op const ret, int_op const fn) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CALL);
    if (ret.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(ret));
    }
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(fn));
    return (*this);
}

auto Program::optailcall(std::string const& fn) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::TAILCALL);
    addr_ptr = encoder.encode_raw_string(addr_ptr, fn);
    return (*this);
}

auto Program::optailcall(int_op const fn) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::TAILCALL);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(fn));
    return (*this);
}

auto Program::opdefer(std::string const& fn) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::DEFER);
    addr_ptr = encoder.encode_raw_string(addr_ptr, fn);
    return (*this);
}

auto Program::opdefer(int_op const fn) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::DEFER);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(fn));
    return (*this);
}

auto Program::opprocess(int_op const ret, std::string const& fn)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::PROCESS);
    if (ret.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(ret));
    }
    addr_ptr = encoder.encode_raw_string(addr_ptr, fn);
    return (*this);
}

auto Program::opprocess(int_op const ret, int_op const fn) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::PROCESS);
    if (ret.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(ret));
    }
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(fn));
    return (*this);
}

auto Program::opself(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SELF);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    return (*this);
}

auto Program::oppideq(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::PIDEQ);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opjoin(int_op const target,
                     int_op const source,
                     timeout_op const timeout) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::JOIN);
    if (target.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    }
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    addr_ptr = encoder.encode_timeout(addr_ptr, timeout.value);
    return (*this);
}

auto Program::opsend(int_op const target, int_op const source) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::SEND);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    return (*this);
}

auto Program::opreceive(int_op const target, timeout_op const timeout) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::RECEIVE);
    if (target.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    }
    addr_ptr = encoder.encode_timeout(addr_ptr, timeout.value);
    return (*this);
}

auto Program::opwatchdog(std::string const& fn) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::WATCHDOG);
    addr_ptr = encoder.encode_raw_string(addr_ptr, fn);
    return (*this);
}

auto Program::opjump(viua::bytecode::codec::bytecode_size_type const addr,
                     enum JUMPTYPE const is_absolute) -> Program&
{
    if (is_absolute != JMP_TO_BYTE) {
        branches.push_back((addr_ptr + 1));
    }

    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::JUMP);
    addr_ptr = encoder.encode_address(addr_ptr, addr);
    return (*this);
}

auto Program::opif(int_op const condition,
                   viua::bytecode::codec::bytecode_size_type const addr_truth,
                   enum JUMPTYPE const absolute_truth,
                   viua::bytecode::codec::bytecode_size_type const addr_false,
                   enum JUMPTYPE const absolute_false) -> Program&
{
    auto jump_position_in_bytecode = addr_ptr;

    jump_position_in_bytecode +=
        sizeof(uint8_t);  // for opcode
    jump_position_in_bytecode +=
        sizeof(uint8_t);  // for operand-type marker
    jump_position_in_bytecode +=
        sizeof(viua::bytecode::codec::Register_set);  // for rs-type marker
    jump_position_in_bytecode += sizeof(viua::bytecode::codec::register_index_type);

    if (absolute_truth != JMP_TO_BYTE) {
        branches.push_back(jump_position_in_bytecode);
    }
    jump_position_in_bytecode += sizeof(viua::bytecode::codec::bytecode_size_type);

    if (absolute_false != JMP_TO_BYTE) {
        branches.push_back(jump_position_in_bytecode);
    }

    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::IF);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(condition));
    addr_ptr = encoder.encode_address(addr_ptr, addr_truth);
    addr_ptr = encoder.encode_address(addr_ptr, addr_false);
    return (*this);
}

auto Program::optry() -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::TRY);
    return (*this);
}

auto Program::opcatch(std::string const tag, std::string const block_name)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::CATCH);
    addr_ptr = encoder.encode_raw_string(addr_ptr, tag.substr(1, (tag.size() - 2)));
    addr_ptr = encoder.encode_raw_string(addr_ptr, block_name);
    return (*this);
}

auto Program::opdraw(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::DRAW);
    if (target.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    }
    return (*this);
}

auto Program::openter(std::string const block_name) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::ENTER);
    addr_ptr = encoder.encode_raw_string(addr_ptr, block_name);
    return (*this);
}

auto Program::opthrow(int_op const source) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::THROW);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    return (*this);
}

auto Program::opleave() -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::LEAVE);
    return (*this);
}

auto Program::opimport(std::string const module_name) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::IMPORT);
    addr_ptr = encoder.encode_raw_string(addr_ptr, module_name);
    return (*this);
}

auto Program::opatom(int_op const target, std::string const& a) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::ATOM);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_raw_string(addr_ptr, a.substr(1, (a.size() - 2)));
    return (*this);
}

auto Program::opatomeq(int_op const target, int_op const lhs, int_op const rhs)
    -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::ATOMEQ);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(lhs));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(rhs));
    return (*this);
}

auto Program::opstruct(int_op const target) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::STRUCT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    return (*this);
}

auto Program::opstructinsert(int_op const target,
                             int_op const key,
                             int_op const source) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::STRUCTINSERT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(key));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    return (*this);
}

auto Program::opstructremove(int_op const target,
                             int_op const key,
                             int_op const source) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::STRUCTREMOVE);
    if (target.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    }
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(key));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    return (*this);
}

auto Program::opstructat(int_op const target,
                         int_op const key,
                         int_op const source) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::STRUCTAT);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(key));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    return (*this);
}

auto Program::opstructkeys(int_op const target, int_op const source) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::STRUCTKEYS);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(target));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(source));
    return (*this);
}

auto Program::op_io_read(int_op const req,
                         int_op const port,
                         int_op const limit) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::IO_READ);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(req));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(port));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(limit));
    return (*this);
}

auto Program::op_io_write(int_op const req,
                          int_op const port,
                          int_op const payload) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::IO_WRITE);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(req));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(port));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(payload));
    return (*this);
}

auto Program::op_io_close(int_op const req, int_op const port) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::IO_CLOSE);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(req));
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(port));
    return (*this);
}

auto Program::op_io_wait(int_op const result,
                         int_op const req,
                         timeout_op limit) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::IO_WAIT);
    if (result.type == Integer_operand_type::VOID) {
        addr_ptr = encoder.encode_void(addr_ptr);
    } else {
        addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(result));
    }
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(req));
    addr_ptr = encoder.encode_timeout(addr_ptr, limit.value);
    return (*this);
}

auto Program::op_io_cancel(int_op const req) -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::IO_CANCEL);
    addr_ptr = encoder.encode_register(addr_ptr, ra_of_intop(req));
    return (*this);
}

auto Program::opreturn() -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::RETURN);
    return (*this);
}

auto Program::ophalt() -> Program&
{
    addr_ptr = encoder.encode_opcode(addr_ptr, OPCODE::HALT);
    return (*this);
}
