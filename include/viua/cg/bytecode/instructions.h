/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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

#ifndef VIUA_CG_BYTECODE_INSTRUCTIONS_H
#define VIUA_CG_BYTECODE_INSTRUCTIONS_H

#pragma once

#include <string>
#include <tuple>
#include <vector>
#include <viua/bytecode/opcodes.h>
#include <viua/support/pointer.h>


enum class IntegerOperandType {
    PLAIN = 0,
    INDEX,
    REGISTER_REFERENCE,
    POINTER_DEREFERENCE,
    VOID,
};

struct int_op {
    IntegerOperandType type;
    viua::internals::RegisterSets rs_type;
    viua::internals::types::plain_int value;

    int_op();
    int_op(IntegerOperandType, viua::internals::types::plain_int = 0);
    int_op(IntegerOperandType,
           viua::internals::RegisterSets,
           viua::internals::types::plain_int = 0);
    int_op(viua::internals::types::plain_int);
};
struct timeout_op {
    IntegerOperandType type;
    viua::internals::types::timeout value;

    timeout_op();
    timeout_op(IntegerOperandType, viua::internals::types::timeout = 0);
    timeout_op(viua::internals::types::timeout);
};

/** typedefs for various types of operands
 *  that Viua asm instructions may use.
 */
typedef std::tuple<bool, viua::internals::types::byte> byte_op;
typedef std::tuple<bool, float> float_op;

namespace cg { namespace bytecode {
auto opnop(viua::internals::types::byte*) -> viua::internals::types::byte*;

auto opizero(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opinteger(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opiinc(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opidec(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;

auto opfloat(viua::internals::types::byte*,
             int_op,
             viua::internals::types::plain_float)
    -> viua::internals::types::byte*;

auto opitof(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opftoi(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opstoi(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opstof(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;

auto opadd(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opsub(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opmul(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opdiv(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto oplt(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto oplte(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opgt(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opgte(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opeq(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;

auto opstring(viua::internals::types::byte*, int_op, std::string)
    -> viua::internals::types::byte*;

auto optext(viua::internals::types::byte*, int_op, std::string)
    -> viua::internals::types::byte*;
auto optext(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto optexteq(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto optextat(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto optextsub(viua::internals::types::byte*, int_op, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto optextlength(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto optextcommonprefix(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto optextcommonsuffix(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto optextconcat(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;

auto opvector(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opvinsert(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opvpush(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opvpop(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opvat(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opvlen(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;

auto opnot(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opand(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opor(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;

auto opbits(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opbits(viua::internals::types::byte*, int_op, std::vector<uint8_t>)
    -> viua::internals::types::byte*;
auto opbitand(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opbitor(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opbitnot(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opbitxor(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opbitat(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opbitset(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opbitset(viua::internals::types::byte*, int_op, int_op, bool)
    -> viua::internals::types::byte*;
auto opshl(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opshr(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opashl(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opashr(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto oprol(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opror(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;

auto opwrapincrement(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opwrapdecrement(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opwrapadd(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opwrapsub(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opwrapmul(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opwrapdiv(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;

auto opcheckedsincrement(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opcheckedsdecrement(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opcheckedsadd(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opcheckedssub(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opcheckedsmul(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opcheckedsdiv(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;

auto opcheckeduincrement(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opcheckedudecrement(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opcheckeduadd(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opcheckedusub(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opcheckedumul(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opcheckedudiv(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;

auto opsaturatingsincrement(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opsaturatingsdecrement(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opsaturatingsadd(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opsaturatingssub(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opsaturatingsmul(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opsaturatingsdiv(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;

auto opsaturatinguincrement(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opsaturatingudecrement(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opsaturatinguadd(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opsaturatingusub(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opsaturatingumul(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opsaturatingudiv(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;

auto opmove(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opcopy(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opptr(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opptrlive(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opswap(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opdelete(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opisnull(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;

auto opprint(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opecho(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;

auto opcapture(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opcapturecopy(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opcapturemove(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opclosure(viua::internals::types::byte*, int_op, std::string const&)
    -> viua::internals::types::byte*;
auto opfunction(viua::internals::types::byte*, int_op, std::string const&)
    -> viua::internals::types::byte*;

auto opframe(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opparam(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto oppamv(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto oparg(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opargc(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opcall(viua::internals::types::byte*, int_op, std::string const&)
    -> viua::internals::types::byte*;
auto opcall(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto optailcall(viua::internals::types::byte*, std::string const&)
    -> viua::internals::types::byte*;
auto optailcall(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opdefer(viua::internals::types::byte*, std::string const&)
    -> viua::internals::types::byte*;
auto opdefer(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opprocess(viua::internals::types::byte*, int_op, std::string const&)
    -> viua::internals::types::byte*;
auto opprocess(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opself(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opjoin(viua::internals::types::byte*, int_op, int_op, timeout_op)
    -> viua::internals::types::byte*;
auto opsend(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opreceive(viua::internals::types::byte*, int_op, timeout_op)
    -> viua::internals::types::byte*;
auto opwatchdog(viua::internals::types::byte*, std::string const&)
    -> viua::internals::types::byte*;

auto opjump(viua::internals::types::byte*,
            viua::internals::types::bytecode_size)
    -> viua::internals::types::byte*;
auto opif(viua::internals::types::byte*,
          int_op,
          viua::internals::types::bytecode_size,
          viua::internals::types::bytecode_size)
    -> viua::internals::types::byte*;

auto optry(viua::internals::types::byte*) -> viua::internals::types::byte*;
auto opcatch(viua::internals::types::byte*,
             std::string const&,
             std::string const&) -> viua::internals::types::byte*;
auto opdraw(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto openter(viua::internals::types::byte*, std::string const&)
    -> viua::internals::types::byte*;
auto opthrow(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opleave(viua::internals::types::byte*) -> viua::internals::types::byte*;

auto opimport(viua::internals::types::byte*, std::string const&)
    -> viua::internals::types::byte*;

auto opclass(viua::internals::types::byte*, int_op, std::string const&)
    -> viua::internals::types::byte*;
auto opderive(viua::internals::types::byte*, int_op, std::string const&)
    -> viua::internals::types::byte*;
auto opattach(viua::internals::types::byte*,
              int_op,
              std::string const&,
              std::string const&) -> viua::internals::types::byte*;
auto opregister(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;

auto opatom(viua::internals::types::byte*, int_op, std::string)
    -> viua::internals::types::byte*;
auto opatomeq(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;

auto opstruct(viua::internals::types::byte*, int_op)
    -> viua::internals::types::byte*;
auto opstructinsert(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opstructremove(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opstructkeys(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;

auto opnew(viua::internals::types::byte*, int_op, std::string const&)
    -> viua::internals::types::byte*;
auto opmsg(viua::internals::types::byte*, int_op, std::string const&)
    -> viua::internals::types::byte*;
auto opmsg(viua::internals::types::byte*, int_op, int_op)
    -> viua::internals::types::byte*;
auto opinsert(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;
auto opremove(viua::internals::types::byte*, int_op, int_op, int_op)
    -> viua::internals::types::byte*;

auto opreturn(viua::internals::types::byte*) -> viua::internals::types::byte*;
auto ophalt(viua::internals::types::byte*) -> viua::internals::types::byte*;
}}  // namespace cg::bytecode

#endif
