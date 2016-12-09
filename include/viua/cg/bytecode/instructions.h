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
    int value;

    int_op();
    int_op(IntegerOperandType, int = 0);
    int_op(int);
};

/** typedefs for various types of operands
 *  that Viua asm instructions may use.
 */
typedef std::tuple<bool, byte> byte_op;
typedef std::tuple<bool, float> float_op;

namespace cg {
    namespace bytecode {
        byte* opnop(byte*);

        byte* opizero(byte*, int_op);
        byte* opistore(byte*, int_op, int_op);
        byte* opiadd(byte*, int_op, int_op, int_op);
        byte* opisub(byte*, int_op, int_op, int_op);
        byte* opimul(byte*, int_op, int_op, int_op);
        byte* opidiv(byte*, int_op, int_op, int_op);
        byte* opiinc(byte*, int_op);
        byte* opidec(byte*, int_op);
        byte* opilt(byte*, int_op, int_op, int_op);
        byte* opilte(byte*, int_op, int_op, int_op);
        byte* opigt(byte*, int_op, int_op, int_op);
        byte* opigte(byte*, int_op, int_op, int_op);
        byte* opieq(byte*, int_op, int_op, int_op);

        byte* opfstore(byte*, int_op, float);
        byte* opfadd(byte*, int_op, int_op, int_op);
        byte* opfsub(byte*, int_op, int_op, int_op);
        byte* opfmul(byte*, int_op, int_op, int_op);
        byte* opfdiv(byte*, int_op, int_op, int_op);
        byte* opflt(byte*, int_op, int_op, int_op);
        byte* opflte(byte*, int_op, int_op, int_op);
        byte* opfgt(byte*, int_op, int_op, int_op);
        byte* opfgte(byte*, int_op, int_op, int_op);
        byte* opfeq(byte*, int_op, int_op, int_op);

        byte* opitof(byte*, int_op, int_op);
        byte* opftoi(byte*, int_op, int_op);
        byte* opstoi(byte*, int_op, int_op);
        byte* opstof(byte*, int_op, int_op);

        byte* opadd(byte*, std::string, int_op, int_op, int_op);
        byte* opsub(byte*, std::string, int_op, int_op, int_op);
        byte* opmul(byte*, std::string, int_op, int_op, int_op);
        byte* opdiv(byte*, std::string, int_op, int_op, int_op);
        byte* oplt(byte*, std::string, int_op, int_op, int_op);
        byte* oplte(byte*, std::string, int_op, int_op, int_op);
        byte* opgt(byte*, std::string, int_op, int_op, int_op);
        byte* opgte(byte*, std::string, int_op, int_op, int_op);
        byte* opeq(byte*, std::string, int_op, int_op, int_op);

        byte* opstrstore(byte*, int_op, std::string);

        byte* opvec(byte*, int_op, int_op, int_op);
        byte* opvinsert(byte*, int_op, int_op, int_op);
        byte* opvpush(byte*, int_op, int_op);
        byte* opvpop(byte*, int_op, int_op, int_op);
        byte* opvat(byte*, int_op, int_op, int_op);
        byte* opvlen(byte*, int_op, int_op);

        byte* opnot(byte*, int_op, int_op);
        byte* opand(byte*, int_op, int_op, int_op);
        byte* opor(byte*, int_op, int_op, int_op);

        byte* opmove(byte*, int_op, int_op);
        byte* opcopy(byte*, int_op, int_op);
        byte* opptr(byte*, int_op, int_op);
        byte* opswap(byte*, int_op, int_op);
        byte* opdelete(byte*, int_op);
        byte* opisnull(byte*, int_op, int_op);
        byte* opress(byte*, const std::string&);
        byte* optmpri(byte*, int_op);
        byte* optmpro(byte*, int_op);

        byte* opprint(byte*, int_op);
        byte* opecho(byte*, int_op);

        byte* opcapture(byte*, int_op, int_op, int_op);
        byte* opcapturecopy(byte*, int_op, int_op, int_op);
        byte* opcapturemove(byte*, int_op, int_op, int_op);
        byte* opclosure(byte*, int_op, const std::string&);
        byte* opfunction(byte*, int_op, const std::string&);
        byte* opfcall(byte*, int_op, int_op);

        byte* opframe(byte*, int_op, int_op);
        byte* opparam(byte*, int_op, int_op);
        byte* oppamv(byte*, int_op, int_op);
        byte* oparg(byte*, int_op, int_op);
        byte* opargc(byte*, int_op);
        byte* opcall(byte*, int_op, const std::string&);
        byte* optailcall(byte*, const std::string&);
        byte* opprocess(byte*, int_op, const std::string&);
        byte* opself(byte*, int_op);
        byte* opjoin(byte*, int_op, int_op, int_op);
        byte* opsend(byte*, int_op, int_op);
        byte* opreceive(byte*, int_op, int_op);
        byte* opwatchdog(byte*, const std::string&);

        byte* opjump(byte*, uint64_t);
        byte* opif(byte*, int_op, uint64_t, uint64_t);

        byte* optry(byte*);
        byte* opcatch(byte*, const std::string&, const std::string&);
        byte* opdraw(byte*, int_op);
        byte* openter(byte*, const std::string&);
        byte* opthrow(byte*, int_op);
        byte* opleave(byte*);

        byte* opimport(byte*, const std::string&);
        byte* oplink(byte*, const std::string&);

        byte* opclass(byte*, int_op, const std::string&);
        byte* opderive(byte*, int_op, const std::string&);
        byte* opattach(byte*, int_op, const std::string&, const std::string&);
        byte* opregister(byte*, int_op);

        byte* opnew(byte*, int_op, const std::string&);
        byte* opmsg(byte*, int_op, const std::string&);
        byte* opinsert(byte*, int_op, int_op, int_op);
        byte* opremove(byte*, int_op, int_op, int_op);

        byte* opreturn(byte*);
        byte* ophalt(byte*);
    }
}

#endif
