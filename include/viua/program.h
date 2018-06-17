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

#ifndef VIUA_PROGRAM_H
#define VIUA_PROGRAM_H

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
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
    std::unique_ptr<viua::internals::types::byte[]> program;
    // size of the bytecode
    viua::internals::types::bytecode_size bytes;

    /** Current address inside bytecode array.
     *  Used during bytecode generation.
     *  Methods implementing generation of specific bytecodes always
     *  append bytes to this pointer.
     */
    viua::internals::types::byte* addr_ptr;

    /** Branches inside bytecode must be stored for later recalculation.
     *  Jumps must be recalculated because each function is compiled separately
     * with jump offset 0, but when they are assembled into a single binary the
     * offsets change.
     */
    std::vector<viua::internals::types::byte*> branches;

    // simple, whether to print debugging information or not
    bool debug;
    bool scream;

  public:
    // instruction insertion interface
    Program& opnop();

    Program& opizero(int_op);
    Program& opinteger(int_op, int_op);
    Program& opiinc(int_op);
    Program& opidec(int_op);

    Program& opfloat(int_op, viua::internals::types::plain_float);

    Program& opitof(int_op, int_op);
    Program& opftoi(int_op, int_op);
    Program& opstoi(int_op, int_op);
    Program& opstof(int_op, int_op);

    Program& opadd(int_op, int_op, int_op);
    Program& opsub(int_op, int_op, int_op);
    Program& opmul(int_op, int_op, int_op);
    Program& opdiv(int_op, int_op, int_op);

    Program& oplt(int_op, int_op, int_op);
    Program& oplte(int_op, int_op, int_op);
    Program& opgt(int_op, int_op, int_op);
    Program& opgte(int_op, int_op, int_op);
    Program& opeq(int_op, int_op, int_op);

    Program& opstring(int_op, std::string);

    Program& optext(int_op, std::string);
    Program& optext(int_op, int_op);
    Program& optexteq(int_op, int_op, int_op);
    Program& optextat(int_op, int_op, int_op);
    Program& optextsub(int_op, int_op, int_op, int_op);
    Program& optextlength(int_op, int_op);
    Program& optextcommonprefix(int_op, int_op, int_op);
    Program& optextcommonsuffix(int_op, int_op, int_op);
    Program& optextconcat(int_op, int_op, int_op);

    Program& opvector(int_op, int_op, int_op);
    Program& opvinsert(int_op, int_op, int_op);
    Program& opvpush(int_op, int_op);
    Program& opvpop(int_op, int_op, int_op);
    Program& opvat(int_op, int_op, int_op);
    Program& opvlen(int_op, int_op);

    Program& opnot(int_op, int_op);
    Program& opand(int_op, int_op, int_op);
    Program& opor(int_op, int_op, int_op);

    Program& opbits(int_op, int_op);
    Program& opbits(int_op, std::vector<uint8_t> const);
    Program& opbitand(int_op, int_op, int_op);
    Program& opbitor(int_op, int_op, int_op);
    Program& opbitnot(int_op, int_op);
    Program& opbitxor(int_op, int_op, int_op);
    Program& opbitat(int_op, int_op, int_op);
    Program& opbitset(int_op, int_op, int_op);
    Program& opbitset(int_op, int_op, bool);
    Program& opshl(int_op, int_op, int_op);
    Program& opshr(int_op, int_op, int_op);
    Program& opashl(int_op, int_op, int_op);
    Program& opashr(int_op, int_op, int_op);
    Program& oprol(int_op, int_op);
    Program& opror(int_op, int_op);

    Program& opwrapincrement(int_op);
    Program& opwrapdecrement(int_op);
    Program& opwrapadd(int_op, int_op, int_op);
    Program& opwrapsub(int_op, int_op, int_op);
    Program& opwrapmul(int_op, int_op, int_op);
    Program& opwrapdiv(int_op, int_op, int_op);

    Program& opcheckedsincrement(int_op);
    Program& opcheckedsdecrement(int_op);
    Program& opcheckedsadd(int_op, int_op, int_op);
    Program& opcheckedssub(int_op, int_op, int_op);
    Program& opcheckedsmul(int_op, int_op, int_op);
    Program& opcheckedsdiv(int_op, int_op, int_op);

    Program& opcheckeduincrement(int_op);
    Program& opcheckedudecrement(int_op);
    Program& opcheckeduadd(int_op, int_op, int_op);
    Program& opcheckedusub(int_op, int_op, int_op);
    Program& opcheckedumul(int_op, int_op, int_op);
    Program& opcheckedudiv(int_op, int_op, int_op);

    Program& opsaturatingsincrement(int_op);
    Program& opsaturatingsdecrement(int_op);
    Program& opsaturatingsadd(int_op, int_op, int_op);
    Program& opsaturatingssub(int_op, int_op, int_op);
    Program& opsaturatingsmul(int_op, int_op, int_op);
    Program& opsaturatingsdiv(int_op, int_op, int_op);

    Program& opsaturatinguincrement(int_op);
    Program& opsaturatingudecrement(int_op);
    Program& opsaturatinguadd(int_op, int_op, int_op);
    Program& opsaturatingusub(int_op, int_op, int_op);
    Program& opsaturatingumul(int_op, int_op, int_op);
    Program& opsaturatingudiv(int_op, int_op, int_op);

    Program& opmove(int_op, int_op);
    Program& opcopy(int_op, int_op);
    Program& opptr(int_op, int_op);
    Program& opptrlive(int_op, int_op);
    Program& opswap(int_op, int_op);
    Program& opdelete(int_op);
    Program& opisnull(int_op, int_op);

    Program& opprint(int_op);
    Program& opecho(int_op);

    Program& opcapture(int_op, int_op, int_op);
    Program& opcapturecopy(int_op, int_op, int_op);
    Program& opcapturemove(int_op, int_op, int_op);
    Program& opclosure(int_op, std::string const&);

    Program& opfunction(int_op, std::string const&);

    Program& opframe(int_op, int_op);
    Program& opparam(int_op, int_op);
    Program& oppamv(int_op, int_op);
    Program& oparg(int_op, int_op);
    Program& opargc(int_op);

    Program& opcall(int_op, std::string const&);
    Program& opcall(int_op, int_op);
    Program& optailcall(std::string const&);
    Program& optailcall(int_op);
    Program& opdefer(std::string const&);
    Program& opdefer(int_op);
    Program& opprocess(int_op, std::string const&);
    Program& opprocess(int_op, int_op);
    Program& opself(int_op);
    Program& opjoin(int_op, int_op, timeout_op);
    Program& opsend(int_op, int_op);
    Program& opreceive(int_op, timeout_op);
    Program& opwatchdog(std::string const&);
    Program& opjump(viua::internals::types::bytecode_size, enum JUMPTYPE);
    Program& opif(int_op,
                  viua::internals::types::bytecode_size,
                  enum JUMPTYPE,
                  viua::internals::types::bytecode_size,
                  enum JUMPTYPE);

    Program& optry();
    Program& opcatch(std::string, std::string);
    Program& opdraw(int_op);
    Program& openter(std::string);
    Program& opthrow(int_op);
    Program& opleave();

    Program& opimport(std::string);

    Program& opatom(int_op, std::string const&);
    Program& opatomeq(int_op, int_op, int_op);

    Program& opstruct(int_op);
    Program& opstructinsert(int_op, int_op, int_op);
    Program& opstructremove(int_op, int_op, int_op);
    Program& opstructkeys(int_op, int_op);

    Program& opreturn();
    Program& ophalt();


    /** Functions driving after-insertion calculations.
     *  These must be called after the bytecode is already generated as they
     * must know size of the program.
     */
    Program& calculate_jumps(
        std::vector<std::tuple<viua::internals::types::bytecode_size,
                               viua::internals::types::bytecode_size>>,
        std::vector<viua::cg::lex::Token>&);
    std::vector<viua::internals::types::bytecode_size> jumps();

    auto bytecode() const -> std::unique_ptr<viua::internals::types::byte[]>;
    Program& fill(std::unique_ptr<viua::internals::types::byte[]>);

    Program& setdebug(bool d = true);
    Program& setscream(bool d = true);

    viua::internals::types::bytecode_size size();

    Program(viua::internals::types::bytecode_size bts = 2);
    Program(Program const& that);
    Program& operator=(Program const& that);
};


#endif
