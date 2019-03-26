/*
 *  Copyright (C) 2015-2019 Marek Marecki
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

  public:
    // instruction insertion interface
    auto opnop() -> Program&;

    auto opizero(int_op const) -> Program&;
    auto opinteger(int_op const, int_op const) -> Program&;
    auto opiinc(int_op const) -> Program&;
    auto opidec(int_op const) -> Program&;

    auto opfloat(int_op const, viua::internals::types::plain_float const)
        -> Program&;

    auto opitof(int_op const, int_op const) -> Program&;
    auto opftoi(int_op const, int_op const) -> Program&;
    auto opstoi(int_op const, int_op const) -> Program&;
    auto opstof(int_op const, int_op const) -> Program&;

    auto opadd(int_op const, int_op const, int_op const) -> Program&;
    auto opsub(int_op const, int_op const, int_op const) -> Program&;
    auto opmul(int_op const, int_op const, int_op const) -> Program&;
    auto opdiv(int_op const, int_op const, int_op const) -> Program&;

    auto oplt(int_op const, int_op const, int_op const) -> Program&;
    auto oplte(int_op const, int_op const, int_op const) -> Program&;
    auto opgt(int_op const, int_op const, int_op const) -> Program&;
    auto opgte(int_op const, int_op const, int_op const) -> Program&;
    auto opeq(int_op const, int_op const, int_op const) -> Program&;

    auto opstring(int_op const, std::string const) -> Program&;

    auto optext(int_op const, std::string const) -> Program&;
    auto optext(int_op const, int_op const) -> Program&;
    auto optexteq(int_op const, int_op const, int_op const) -> Program&;
    auto optextat(int_op const, int_op const, int_op const) -> Program&;
    auto optextsub(int_op const, int_op const, int_op const, int_op const)
        -> Program&;
    auto optextlength(int_op const, int_op const) -> Program&;
    auto optextcommonprefix(int_op const, int_op const, int_op const)
        -> Program&;
    auto optextcommonsuffix(int_op const, int_op const, int_op const)
        -> Program&;
    auto optextconcat(int_op const, int_op const, int_op const) -> Program&;

    auto opvector(int_op const, int_op const, int_op const) -> Program&;
    auto opvinsert(int_op const, int_op const, int_op const) -> Program&;
    auto opvpush(int_op const, int_op const) -> Program&;
    auto opvpop(int_op const, int_op const, int_op const) -> Program&;
    auto opvat(int_op const, int_op const, int_op const) -> Program&;
    auto opvlen(int_op const, int_op const) -> Program&;

    auto opnot(int_op const, int_op const) -> Program&;
    auto opand(int_op const, int_op const, int_op const) -> Program&;
    auto opor(int_op const, int_op const, int_op const) -> Program&;

    auto opbits(int_op const, int_op const) -> Program&;
    auto opbits(int_op const, std::vector<uint8_t> const) -> Program&;
    auto opbitand(int_op const, int_op const, int_op const) -> Program&;
    auto opbitor(int_op const, int_op const, int_op const) -> Program&;
    auto opbitnot(int_op const, int_op const) -> Program&;
    auto opbitxor(int_op const, int_op const, int_op const) -> Program&;
    auto opbitat(int_op const, int_op const, int_op const) -> Program&;
    auto opbitset(int_op const, int_op const, int_op const) -> Program&;
    auto opbitset(int_op const, int_op const, bool const) -> Program&;
    auto opshl(int_op const, int_op const, int_op const) -> Program&;
    auto opshr(int_op const, int_op const, int_op const) -> Program&;
    auto opashl(int_op const, int_op const, int_op const) -> Program&;
    auto opashr(int_op const, int_op const, int_op const) -> Program&;
    auto oprol(int_op const, int_op const) -> Program&;
    auto opror(int_op const, int_op const) -> Program&;

    auto opwrapincrement(int_op const) -> Program&;
    auto opwrapdecrement(int_op const) -> Program&;
    auto opwrapadd(int_op const, int_op const, int_op const) -> Program&;
    auto opwrapsub(int_op const, int_op const, int_op const) -> Program&;
    auto opwrapmul(int_op const, int_op const, int_op const) -> Program&;
    auto opwrapdiv(int_op const, int_op const, int_op const) -> Program&;

    auto opcheckedsincrement(int_op const) -> Program&;
    auto opcheckedsdecrement(int_op const) -> Program&;
    auto opcheckedsadd(int_op const, int_op const, int_op const) -> Program&;
    auto opcheckedssub(int_op const, int_op const, int_op const) -> Program&;
    auto opcheckedsmul(int_op const, int_op const, int_op const) -> Program&;
    auto opcheckedsdiv(int_op const, int_op const, int_op const) -> Program&;

    auto opcheckeduincrement(int_op const) -> Program&;
    auto opcheckedudecrement(int_op const) -> Program&;
    auto opcheckeduadd(int_op const, int_op const, int_op const) -> Program&;
    auto opcheckedusub(int_op const, int_op const, int_op const) -> Program&;
    auto opcheckedumul(int_op const, int_op const, int_op const) -> Program&;
    auto opcheckedudiv(int_op const, int_op const, int_op const) -> Program&;

    auto opsaturatingsincrement(int_op const) -> Program&;
    auto opsaturatingsdecrement(int_op const) -> Program&;
    auto opsaturatingsadd(int_op const, int_op const, int_op const) -> Program&;
    auto opsaturatingssub(int_op const, int_op const, int_op const) -> Program&;
    auto opsaturatingsmul(int_op const, int_op const, int_op const) -> Program&;
    auto opsaturatingsdiv(int_op const, int_op const, int_op const) -> Program&;

    auto opsaturatinguincrement(int_op const) -> Program&;
    auto opsaturatingudecrement(int_op const) -> Program&;
    auto opsaturatinguadd(int_op const, int_op const, int_op const) -> Program&;
    auto opsaturatingusub(int_op const, int_op const, int_op const) -> Program&;
    auto opsaturatingumul(int_op const, int_op const, int_op const) -> Program&;
    auto opsaturatingudiv(int_op const, int_op const, int_op const) -> Program&;

    auto opmove(int_op const, int_op const) -> Program&;
    auto opcopy(int_op const, int_op const) -> Program&;
    auto opptr(int_op const, int_op const) -> Program&;
    auto opptrlive(int_op const, int_op const) -> Program&;
    auto opswap(int_op const, int_op const) -> Program&;
    auto opdelete(int_op const) -> Program&;
    auto opisnull(int_op const, int_op const) -> Program&;

    auto opprint(int_op const) -> Program&;
    auto opecho(int_op const) -> Program&;

    auto opcapture(int_op const, int_op const, int_op const) -> Program&;
    auto opcapturecopy(int_op const, int_op const, int_op const) -> Program&;
    auto opcapturemove(int_op const, int_op const, int_op const) -> Program&;
    auto opclosure(int_op const, std::string const&) -> Program&;

    auto opfunction(int_op const, std::string const&) -> Program&;

    auto opframe(int_op const, int_op const) -> Program&;
    auto opallocate_registers(int_op const) -> Program&;

    auto opcall(int_op const, std::string const&) -> Program&;
    auto opcall(int_op const, int_op const) -> Program&;
    auto optailcall(std::string const&) -> Program&;
    auto optailcall(int_op const) -> Program&;
    auto opdefer(std::string const&) -> Program&;
    auto opdefer(int_op const) -> Program&;
    auto opprocess(int_op const, std::string const&) -> Program&;
    auto opprocess(int_op const, int_op const) -> Program&;
    auto opself(int_op const) -> Program&;
    auto oppideq(int_op const, int_op const, int_op const) -> Program&;
    auto opjoin(int_op const, int_op const, timeout_op) -> Program&;
    auto opsend(int_op const, int_op const) -> Program&;
    auto opreceive(int_op const, timeout_op) -> Program&;
    auto opwatchdog(std::string const&) -> Program&;
    auto opjump(viua::internals::types::bytecode_size const,
                enum JUMPTYPE const) -> Program&;
    auto opif(int_op const,
              viua::internals::types::bytecode_size const,
              enum JUMPTYPE const,
              viua::internals::types::bytecode_size const,
              enum JUMPTYPE const) -> Program&;

    auto optry() -> Program&;
    auto opcatch(std::string const, std::string const) -> Program&;
    auto opdraw(int_op const) -> Program&;
    auto openter(std::string const) -> Program&;
    auto opthrow(int_op const) -> Program&;
    auto opleave() -> Program&;

    auto opimport(std::string const) -> Program&;

    auto opatom(int_op const, std::string const&) -> Program&;
    auto opatomeq(int_op const, int_op const, int_op const) -> Program&;

    auto opstruct(int_op const) -> Program&;
    auto opstructinsert(int_op const, int_op const, int_op const) -> Program&;
    auto opstructremove(int_op const, int_op const, int_op const) -> Program&;
    auto opstructat(int_op const, int_op const, int_op const) -> Program&;
    auto opstructkeys(int_op const, int_op const) -> Program&;

    auto opreturn() -> Program&;
    auto ophalt() -> Program&;


    /** Functions driving after-insertion calculations.
     *  These must be called after the bytecode is already generated as they
     * must know size of the program.
     */
    auto calculate_jumps(
        std::vector<std::tuple<viua::internals::types::bytecode_size,
                               viua::internals::types::bytecode_size>> const,
        std::vector<viua::cg::lex::Token> const&) -> Program&;
    auto jumps() const -> std::vector<viua::internals::types::bytecode_size>;

    auto bytecode() const -> std::unique_ptr<viua::internals::types::byte[]>;
    auto fill(std::unique_ptr<viua::internals::types::byte[]>) -> Program&;

    auto size() const -> viua::internals::types::bytecode_size;

    Program(viua::internals::types::bytecode_size const bts = 2);
    Program(Program const& that);
    Program(Program&&) = delete;
    auto operator=(Program const&) -> Program& = delete;
    auto operator=(Program &&) -> Program& = delete;
};


#endif
