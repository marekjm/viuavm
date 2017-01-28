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

#ifndef VIUA_PROGRAM_H
#define VIUA_PROGRAM_H

#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <viua/bytecode/bytetypedef.h>
#include <viua/cg/bytecode/instructions.h>
#include <viua/cg/lex.h>


enum JUMPTYPE {
    JMP_RELATIVE = 0,
    JMP_ABSOLUTE,
    JMP_TO_BYTE,
};


class Program {
    // byte array containing bytecode
    viua::internals::types::byte* program;
    // size of the bytecode
    viua::internals::types::bytecode_size bytes;

    /** Current address inside bytecode array.
     *  Used during bytecode generation.
     *  Methods implementing generation of specific bytecodes always
     *  append bytes to this pointer.
     */
    viua::internals::types::byte* addr_ptr;

    /** Branches inside bytecode must be stored for later recalculation.
     *  Jumps must be recalculated because each function is compiled separately with jump offset 0, but
     *  when they are assembled into a single binary the offsets change.
     */
    std::vector<viua::internals::types::byte*> branches;

    // simple, whether to print debugging information or not
    bool debug;
    bool scream;

    public:
    // instruction insertion interface
    Program& opnop();

    Program& opizero(int_op);
    Program& opistore(int_op, int_op);
    Program& opiinc(int_op);
    Program& opidec(int_op);

    Program& opfstore(int_op, viua::internals::types::plain_float);

    Program& opitof(int_op, int_op);
    Program& opftoi(int_op, int_op);
    Program& opstoi(int_op, int_op);
    Program& opstof(int_op, int_op);

    Program& opadd(std::string, int_op, int_op, int_op);
    Program& opsub(std::string, int_op, int_op, int_op);
    Program& opmul(std::string, int_op, int_op, int_op);
    Program& opdiv(std::string, int_op, int_op, int_op);

    Program& oplt(std::string, int_op, int_op, int_op);
    Program& oplte(std::string, int_op, int_op, int_op);
    Program& opgt(std::string, int_op, int_op, int_op);
    Program& opgte(std::string, int_op, int_op, int_op);
    Program& opeq(std::string, int_op, int_op, int_op);

    Program& opstrstore(int_op, std::string);

    Program& opvec(int_op, int_op, int_op);
    Program& opvinsert(int_op, int_op, int_op);
    Program& opvpush(int_op, int_op);
    Program& opvpop(int_op, int_op, int_op);
    Program& opvat(int_op, int_op, int_op);
    Program& opvlen(int_op, int_op);

    Program& opnot(int_op, int_op);
    Program& opand(int_op, int_op, int_op);
    Program& opor(int_op, int_op, int_op);

    Program& opmove(int_op, int_op);
    Program& opcopy(int_op, int_op);
    Program& opptr(int_op, int_op);
    Program& opswap(int_op, int_op);
    Program& opress(std::string);
    Program& optmpri(int_op);
    Program& optmpro(int_op);
    Program& opdelete(int_op);
    Program& opisnull(int_op, int_op);

    Program& opprint(int_op);
    Program& opecho(int_op);

    Program& opcapture(int_op, int_op, int_op);
    Program& opcapturecopy(int_op, int_op, int_op);
    Program& opcapturemove(int_op, int_op, int_op);
    Program& opclosure(int_op, const std::string&);

    Program& opfunction(int_op, const std::string&);
    Program& opfcall(int_op, int_op);

    Program& opframe(int_op, int_op);
    Program& opparam(int_op, int_op);
    Program& oppamv(int_op, int_op);
    Program& oparg(int_op, int_op);
    Program& opargc(int_op);

    Program& opcall(int_op, const std::string&);
    Program& optailcall(const std::string&);
    Program& opprocess(int_op, const std::string&);
    Program& opself(int_op);
    Program& opjoin(int_op, int_op, timeout_op);
    Program& opsend(int_op, int_op);
    Program& opreceive(int_op, timeout_op);
    Program& opwatchdog(const std::string&);
    Program& opjump(viua::internals::types::bytecode_size, enum JUMPTYPE);
    Program& opif(int_op, viua::internals::types::bytecode_size, enum JUMPTYPE, viua::internals::types::bytecode_size, enum JUMPTYPE);

    Program& optry();
    Program& opcatch(std::string, std::string);
    Program& opdraw(int_op);
    Program& openter(std::string);
    Program& opthrow(int_op);
    Program& opleave();

    Program& opimport(std::string);
    Program& oplink(std::string);

    Program& opclass(int_op, const std::string&);
    Program& opderive(int_op, const std::string&);
    Program& opattach(int_op, const std::string&, const std::string&);
    Program& opregister(int_op);

    Program& opnew(int_op, const std::string&);
    Program& opmsg(int_op, const std::string&);
    Program& opinsert(int_op, int_op, int_op);
    Program& opremove(int_op, int_op, int_op);

    Program& opreturn();
    Program& ophalt();


    /** Functions driving after-insertion calculations.
     *  These must be called after the bytecode is already generated as they must know
     *  size of the program.
     */
    Program& calculateJumps(std::vector<std::tuple<viua::internals::types::bytecode_size, viua::internals::types::bytecode_size>>, std::vector<viua::cg::lex::Token>&);
    std::vector<viua::internals::types::bytecode_size> jumps();

    viua::internals::types::byte* bytecode();
    Program& fill(viua::internals::types::byte*);

    Program& setdebug(bool d = true);
    Program& setscream(bool d = true);

    viua::internals::types::bytecode_size size();

    Program(viua::internals::types::bytecode_size bts = 2);
    Program(const Program& that);
    ~Program();
    Program& operator=(const Program& that);
};


#endif
