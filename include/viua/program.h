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
    byte* program;
    // size of the bytecode
    uint64_t bytes;

    /** Current address inside bytecode array.
     *  Used during bytecode generation.
     *  Methods implementing generation of specific bytecodes always
     *  append bytes to this pointer.
     */
    byte* addr_ptr;

    /** Branches inside bytecode must be stored for later recalculation.
     *  Absolute and local branches must be distinguished between as
     *  they are calculated a bit differently.
     */
    std::vector<byte*> branches;
    std::vector<byte*> branches_absolute;

    // simple, whether to print debugging information or not
    bool debug;
    bool scream;

    uint64_t getInstructionBytecodeOffset(uint64_t, uint64_t count = 0);

    public:
    // instruction insertion interface
    Program& opnop();

    Program& opizero(int_op);
    Program& opistore(int_op, int_op);
    Program& opiadd(int_op, int_op, int_op);
    Program& opisub(int_op, int_op, int_op);
    Program& opimul(int_op, int_op, int_op);
    Program& opidiv(int_op, int_op, int_op);

    Program& opiinc(int_op);
    Program& opidec(int_op);

    Program& opilt(int_op, int_op, int_op);
    Program& opilte(int_op, int_op, int_op);
    Program& opigt(int_op, int_op, int_op);
    Program& opigte(int_op, int_op, int_op);
    Program& opieq(int_op, int_op, int_op);

    Program& opfstore(int_op, float);
    Program& opfadd(int_op, int_op, int_op);
    Program& opfsub(int_op, int_op, int_op);
    Program& opfmul(int_op, int_op, int_op);
    Program& opfdiv(int_op, int_op, int_op);

    Program& opflt(int_op, int_op, int_op);
    Program& opflte(int_op, int_op, int_op);
    Program& opfgt(int_op, int_op, int_op);
    Program& opfgte(int_op, int_op, int_op);
    Program& opfeq(int_op, int_op, int_op);

    Program& opitof(int_op, int_op);
    Program& opftoi(int_op, int_op);
    Program& opstoi(int_op, int_op);
    Program& opstof(int_op, int_op);

    Program& opstrstore(int_op, std::string);

    Program& opvec(int_op, int_op, int_op);
    Program& opvinsert(int_op, int_op, int_op);
    Program& opvpush(int_op, int_op);
    Program& opvpop(int_op, int_op, int_op);
    Program& opvat(int_op, int_op, int_op);
    Program& opvlen(int_op, int_op);

    Program& opnot(int_op);
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

    Program& openclose(int_op, int_op, int_op);
    Program& openclosecopy(int_op, int_op, int_op);
    Program& openclosemove(int_op, int_op, int_op);
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
    Program& opjoin(int_op, int_op, int_op);
    Program& opsend(int_op, int_op);
    Program& opreceive(int_op, int_op);
    Program& opwatchdog(const std::string&);
    Program& opjump(uint64_t, enum JUMPTYPE);
    Program& opif(int_op, uint64_t, enum JUMPTYPE, uint64_t, enum JUMPTYPE);

    Program& optry();
    Program& opcatch(std::string, std::string);
    Program& oppull(int_op);
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
    Program& calculateJumps(std::vector<std::tuple<uint64_t, uint64_t>>, std::vector<viua::cg::lex::Token>&);
    std::vector<uint64_t> jumps();

    byte* bytecode();
    Program& fill(byte*);

    Program& setdebug(bool d = true);
    Program& setscream(bool d = true);

    uint64_t size();
    unsigned long instructionCount();

    Program(uint64_t bts = 2);
    Program(const Program& that);
    ~Program();
    Program& operator=(const Program& that);
};


#endif
