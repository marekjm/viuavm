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
    auto opnop() -> Program&;

    auto opizero(int_op) -> Program&;
    auto opinteger(int_op, int_op) -> Program&;
    auto opiinc(int_op) -> Program&;
    auto opidec(int_op) -> Program&;

    auto opfloat(int_op, viua::internals::types::plain_float) -> Program&;

    auto opitof(int_op, int_op) -> Program&;
    auto opftoi(int_op, int_op) -> Program&;
    auto opstoi(int_op, int_op) -> Program&;
    auto opstof(int_op, int_op) -> Program&;

    auto opadd(int_op, int_op, int_op) -> Program&;
    auto opsub(int_op, int_op, int_op) -> Program&;
    auto opmul(int_op, int_op, int_op) -> Program&;
    auto opdiv(int_op, int_op, int_op) -> Program&;

    auto oplt(int_op, int_op, int_op) -> Program&;
    auto oplte(int_op, int_op, int_op) -> Program&;
    auto opgt(int_op, int_op, int_op) -> Program&;
    auto opgte(int_op, int_op, int_op) -> Program&;
    auto opeq(int_op, int_op, int_op) -> Program&;

    auto opstring(int_op, std::string) -> Program&;

    auto optext(int_op, std::string) -> Program&;
    auto optext(int_op, int_op) -> Program&;
    auto optexteq(int_op, int_op, int_op) -> Program&;
    auto optextat(int_op, int_op, int_op) -> Program&;
    auto optextsub(int_op, int_op, int_op, int_op) -> Program&;
    auto optextlength(int_op, int_op) -> Program&;
    auto optextcommonprefix(int_op, int_op, int_op) -> Program&;
    auto optextcommonsuffix(int_op, int_op, int_op) -> Program&;
    auto optextconcat(int_op, int_op, int_op) -> Program&;

    auto opvector(int_op, int_op, int_op) -> Program&;
    auto opvinsert(int_op, int_op, int_op) -> Program&;
    auto opvpush(int_op, int_op) -> Program&;
    auto opvpop(int_op, int_op, int_op) -> Program&;
    auto opvat(int_op, int_op, int_op) -> Program&;
    auto opvlen(int_op, int_op) -> Program&;

    auto opnot(int_op, int_op) -> Program&;
    auto opand(int_op, int_op, int_op) -> Program&;
    auto opor(int_op, int_op, int_op) -> Program&;

    auto opbits(int_op, int_op) -> Program&;
    auto opbits(int_op, std::vector<uint8_t> const) -> Program&;
    auto opbitand(int_op, int_op, int_op) -> Program&;
    auto opbitor(int_op, int_op, int_op) -> Program&;
    auto opbitnot(int_op, int_op) -> Program&;
    auto opbitxor(int_op, int_op, int_op) -> Program&;
    auto opbitat(int_op, int_op, int_op) -> Program&;
    auto opbitset(int_op, int_op, int_op) -> Program&;
    auto opbitset(int_op, int_op, bool) -> Program&;
    auto opshl(int_op, int_op, int_op) -> Program&;
    auto opshr(int_op, int_op, int_op) -> Program&;
    auto opashl(int_op, int_op, int_op) -> Program&;
    auto opashr(int_op, int_op, int_op) -> Program&;
    auto oprol(int_op, int_op) -> Program&;
    auto opror(int_op, int_op) -> Program&;

    auto opwrapincrement(int_op) -> Program&;
    auto opwrapdecrement(int_op) -> Program&;
    auto opwrapadd(int_op, int_op, int_op) -> Program&;
    auto opwrapsub(int_op, int_op, int_op) -> Program&;
    auto opwrapmul(int_op, int_op, int_op) -> Program&;
    auto opwrapdiv(int_op, int_op, int_op) -> Program&;

    auto opcheckedsincrement(int_op) -> Program&;
    auto opcheckedsdecrement(int_op) -> Program&;
    auto opcheckedsadd(int_op, int_op, int_op) -> Program&;
    auto opcheckedssub(int_op, int_op, int_op) -> Program&;
    auto opcheckedsmul(int_op, int_op, int_op) -> Program&;
    auto opcheckedsdiv(int_op, int_op, int_op) -> Program&;

    auto opcheckeduincrement(int_op) -> Program&;
    auto opcheckedudecrement(int_op) -> Program&;
    auto opcheckeduadd(int_op, int_op, int_op) -> Program&;
    auto opcheckedusub(int_op, int_op, int_op) -> Program&;
    auto opcheckedumul(int_op, int_op, int_op) -> Program&;
    auto opcheckedudiv(int_op, int_op, int_op) -> Program&;

    auto opsaturatingsincrement(int_op) -> Program&;
    auto opsaturatingsdecrement(int_op) -> Program&;
    auto opsaturatingsadd(int_op, int_op, int_op) -> Program&;
    auto opsaturatingssub(int_op, int_op, int_op) -> Program&;
    auto opsaturatingsmul(int_op, int_op, int_op) -> Program&;
    auto opsaturatingsdiv(int_op, int_op, int_op) -> Program&;

    auto opsaturatinguincrement(int_op) -> Program&;
    auto opsaturatingudecrement(int_op) -> Program&;
    auto opsaturatinguadd(int_op, int_op, int_op) -> Program&;
    auto opsaturatingusub(int_op, int_op, int_op) -> Program&;
    auto opsaturatingumul(int_op, int_op, int_op) -> Program&;
    auto opsaturatingudiv(int_op, int_op, int_op) -> Program&;

    auto opmove(int_op, int_op) -> Program&;
    auto opcopy(int_op, int_op) -> Program&;
    auto opptr(int_op, int_op) -> Program&;
    auto opptrlive(int_op, int_op) -> Program&;
    auto opswap(int_op, int_op) -> Program&;
    auto opdelete(int_op) -> Program&;
    auto opisnull(int_op, int_op) -> Program&;

    auto opprint(int_op) -> Program&;
    auto opecho(int_op) -> Program&;

    auto opcapture(int_op, int_op, int_op) -> Program&;
    auto opcapturecopy(int_op, int_op, int_op) -> Program&;
    auto opcapturemove(int_op, int_op, int_op) -> Program&;
    auto opclosure(int_op, std::string const&) -> Program&;

    auto opfunction(int_op, std::string const&) -> Program&;

    auto opframe(int_op, int_op) -> Program&;
    auto opparam(int_op, int_op) -> Program&;
    auto oppamv(int_op, int_op) -> Program&;
    auto oparg(int_op, int_op) -> Program&;
    auto opargc(int_op) -> Program&;
    auto opallocate_registers(int_op) -> Program&;

    auto opcall(int_op, std::string const&) -> Program&;
    auto opcall(int_op, int_op) -> Program&;
    auto optailcall(std::string const&) -> Program&;
    auto optailcall(int_op) -> Program&;
    auto opdefer(std::string const&) -> Program&;
    auto opdefer(int_op) -> Program&;
    auto opprocess(int_op, std::string const&) -> Program&;
    auto opprocess(int_op, int_op) -> Program&;
    auto opself(int_op) -> Program&;
    auto opjoin(int_op, int_op, timeout_op) -> Program&;
    auto opsend(int_op, int_op) -> Program&;
    auto opreceive(int_op, timeout_op) -> Program&;
    auto opwatchdog(std::string const&) -> Program&;
    auto opjump(viua::internals::types::bytecode_size, enum JUMPTYPE) -> Program&;
    auto opif(int_op, viua::internals::types::bytecode_size, enum JUMPTYPE, viua::internals::types::bytecode_size, enum JUMPTYPE) -> Program&;

    auto optry() -> Program&;
    auto opcatch(std::string, std::string) -> Program&;
    auto opdraw(int_op) -> Program&;
    auto openter(std::string) -> Program&;
    auto opthrow(int_op) -> Program&;
    auto opleave() -> Program&;

    auto opimport(std::string) -> Program&;

    auto opatom(int_op, std::string const&) -> Program&;
    auto opatomeq(int_op, int_op, int_op) -> Program&;

    auto opstruct(int_op) -> Program&;
    auto opstructinsert(int_op, int_op, int_op) -> Program&;
    auto opstructremove(int_op, int_op, int_op) -> Program&;
    auto opstructkeys(int_op, int_op) -> Program&;

    auto opreturn() -> Program&;
    auto ophalt() -> Program&;


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
