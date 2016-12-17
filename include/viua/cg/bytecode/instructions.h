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

#ifndef VIUA_CG_BYTECODE_INSTRUCTIONS_H
#define VIUA_CG_BYTECODE_INSTRUCTIONS_H

#pragma once

#include <string>
#include <tuple>
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
    viua::internals::types::plain_int value;

    int_op();
    int_op(IntegerOperandType, viua::internals::types::plain_int = 0);
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

namespace cg {
    namespace bytecode {
        viua::internals::types::byte* opnop(viua::internals::types::byte*);

        viua::internals::types::byte* opizero(viua::internals::types::byte*, int_op);
        viua::internals::types::byte* opistore(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opiinc(viua::internals::types::byte*, int_op);
        viua::internals::types::byte* opidec(viua::internals::types::byte*, int_op);

        viua::internals::types::byte* opfstore(viua::internals::types::byte*, int_op, float);

        viua::internals::types::byte* opitof(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opftoi(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opstoi(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opstof(viua::internals::types::byte*, int_op, int_op);

        viua::internals::types::byte* opadd(viua::internals::types::byte*, std::string, int_op, int_op, int_op);
        viua::internals::types::byte* opsub(viua::internals::types::byte*, std::string, int_op, int_op, int_op);
        viua::internals::types::byte* opmul(viua::internals::types::byte*, std::string, int_op, int_op, int_op);
        viua::internals::types::byte* opdiv(viua::internals::types::byte*, std::string, int_op, int_op, int_op);
        viua::internals::types::byte* oplt(viua::internals::types::byte*, std::string, int_op, int_op, int_op);
        viua::internals::types::byte* oplte(viua::internals::types::byte*, std::string, int_op, int_op, int_op);
        viua::internals::types::byte* opgt(viua::internals::types::byte*, std::string, int_op, int_op, int_op);
        viua::internals::types::byte* opgte(viua::internals::types::byte*, std::string, int_op, int_op, int_op);
        viua::internals::types::byte* opeq(viua::internals::types::byte*, std::string, int_op, int_op, int_op);

        viua::internals::types::byte* opstrstore(viua::internals::types::byte*, int_op, std::string);

        viua::internals::types::byte* opvec(viua::internals::types::byte*, int_op, int_op, int_op);
        viua::internals::types::byte* opvinsert(viua::internals::types::byte*, int_op, int_op, int_op);
        viua::internals::types::byte* opvpush(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opvpop(viua::internals::types::byte*, int_op, int_op, int_op);
        viua::internals::types::byte* opvat(viua::internals::types::byte*, int_op, int_op, int_op);
        viua::internals::types::byte* opvlen(viua::internals::types::byte*, int_op, int_op);

        viua::internals::types::byte* opnot(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opand(viua::internals::types::byte*, int_op, int_op, int_op);
        viua::internals::types::byte* opor(viua::internals::types::byte*, int_op, int_op, int_op);

        viua::internals::types::byte* opmove(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opcopy(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opptr(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opswap(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opdelete(viua::internals::types::byte*, int_op);
        viua::internals::types::byte* opisnull(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opress(viua::internals::types::byte*, const std::string&);
        viua::internals::types::byte* optmpri(viua::internals::types::byte*, int_op);
        viua::internals::types::byte* optmpro(viua::internals::types::byte*, int_op);

        viua::internals::types::byte* opprint(viua::internals::types::byte*, int_op);
        viua::internals::types::byte* opecho(viua::internals::types::byte*, int_op);

        viua::internals::types::byte* opcapture(viua::internals::types::byte*, int_op, int_op, int_op);
        viua::internals::types::byte* opcapturecopy(viua::internals::types::byte*, int_op, int_op, int_op);
        viua::internals::types::byte* opcapturemove(viua::internals::types::byte*, int_op, int_op, int_op);
        viua::internals::types::byte* opclosure(viua::internals::types::byte*, int_op, const std::string&);
        viua::internals::types::byte* opfunction(viua::internals::types::byte*, int_op, const std::string&);
        viua::internals::types::byte* opfcall(viua::internals::types::byte*, int_op, int_op);

        viua::internals::types::byte* opframe(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opparam(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* oppamv(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* oparg(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opargc(viua::internals::types::byte*, int_op);
        viua::internals::types::byte* opcall(viua::internals::types::byte*, int_op, const std::string&);
        viua::internals::types::byte* optailcall(viua::internals::types::byte*, const std::string&);
        viua::internals::types::byte* opprocess(viua::internals::types::byte*, int_op, const std::string&);
        viua::internals::types::byte* opself(viua::internals::types::byte*, int_op);
        viua::internals::types::byte* opjoin(viua::internals::types::byte*, int_op, int_op, timeout_op);
        viua::internals::types::byte* opsend(viua::internals::types::byte*, int_op, int_op);
        viua::internals::types::byte* opreceive(viua::internals::types::byte*, int_op, timeout_op);
        viua::internals::types::byte* opwatchdog(viua::internals::types::byte*, const std::string&);

        viua::internals::types::byte* opjump(viua::internals::types::byte*, viua::internals::types::bytecode_size);
        viua::internals::types::byte* opif(viua::internals::types::byte*, int_op, viua::internals::types::bytecode_size, viua::internals::types::bytecode_size);

        viua::internals::types::byte* optry(viua::internals::types::byte*);
        viua::internals::types::byte* opcatch(viua::internals::types::byte*, const std::string&, const std::string&);
        viua::internals::types::byte* opdraw(viua::internals::types::byte*, int_op);
        viua::internals::types::byte* openter(viua::internals::types::byte*, const std::string&);
        viua::internals::types::byte* opthrow(viua::internals::types::byte*, int_op);
        viua::internals::types::byte* opleave(viua::internals::types::byte*);

        viua::internals::types::byte* opimport(viua::internals::types::byte*, const std::string&);
        viua::internals::types::byte* oplink(viua::internals::types::byte*, const std::string&);

        viua::internals::types::byte* opclass(viua::internals::types::byte*, int_op, const std::string&);
        viua::internals::types::byte* opderive(viua::internals::types::byte*, int_op, const std::string&);
        viua::internals::types::byte* opattach(viua::internals::types::byte*, int_op, const std::string&, const std::string&);
        viua::internals::types::byte* opregister(viua::internals::types::byte*, int_op);

        viua::internals::types::byte* opnew(viua::internals::types::byte*, int_op, const std::string&);
        viua::internals::types::byte* opmsg(viua::internals::types::byte*, int_op, const std::string&);
        viua::internals::types::byte* opinsert(viua::internals::types::byte*, int_op, int_op, int_op);
        viua::internals::types::byte* opremove(viua::internals::types::byte*, int_op, int_op, int_op);

        viua::internals::types::byte* opreturn(viua::internals::types::byte*);
        viua::internals::types::byte* ophalt(viua::internals::types::byte*);
    }
}

#endif
