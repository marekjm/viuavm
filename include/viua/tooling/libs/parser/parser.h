/*
 *  Copyright (C) 2018 Marek Marecki
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

#ifndef VIUA_TOOLING_LIBS_PARSER_PARSER_H
#define VIUA_TOOLING_LIBS_PARSER_PARSER_H

#include <memory>
#include <string>
#include <vector>
#include <viua/bytecode/operand_types.h>
#include <viua/bytecode/bytetypedef.h>
#include <viua/types/integer.h>
#include <viua/types/float.h>
#include <viua/tooling/libs/lexer/tokenise.h>

namespace viua {
namespace tooling {
namespace libs {
namespace parser {

enum class Fragment_type {
    Signature_directive,
    Block_signature_directive,
    End_directive,
    Closure_head,
    Block_head,
    Instruction,
    Info_directive,
    Import_directive,
};

/*
 * Fragment represents a single element of the program; a function name,
 * an attribute list, a register address, an instruction, etc.
 * Raw tokens are reduced to fragments.
 */
class Fragment {
    std::vector<viua::tooling::libs::lexer::Token> tokens;
    Fragment_type const fragment_type;

  public:
    auto type() const -> Fragment_type;
    auto add(viua::tooling::libs::lexer::Token) -> void;

    Fragment(Fragment_type const);
};

struct Signature_directive : public Fragment {
    std::string const function_name;
    uint64_t const arity;

    Signature_directive(std::string, uint64_t const);
};

struct Block_signature_directive : public Fragment {
    std::string const block_name;

    Block_signature_directive(std::string);
};

struct End_directive : public Fragment {
    End_directive();
};

struct Closure_head : public Fragment {
    std::string const function_name;
    uint64_t const arity;
    std::set<std::string> const attributes;

    Closure_head(std::string, uint64_t const, std::set<std::string>);
};

struct Block_head : public Fragment {
    std::string const name;
    std::set<std::string> const attributes;

    Block_head(std::string, std::set<std::string>);
};

struct Info_directive : public Fragment {
    std::string const key;
    std::string const value;

    Info_directive(std::string, std::string);
};

struct Import_directive : public Fragment {
    std::string const module_name;
    std::set<std::string> const attributes;

    Import_directive(std::string, std::set<std::string>);
};

enum class Operand_type {
    Register_address,
    Integer_literal,
    Float_literal,
    Text_literal,
    Atom_literal,
    Bits_literal,
    Boolean_literal,
    Timeout_literal,
    Void,
    Function_name,
    Block_name,
    Module_name,
    Jump_offset,
    Jump_label,
};

class Operand {
    std::vector<viua::tooling::libs::lexer::Token> tokens;
    Operand_type const operand_type;

  public:
    auto type() const -> Operand_type;
    auto add(viua::tooling::libs::lexer::Token) -> void;

    Operand(Operand_type const);
};

struct Register_address : public Operand {
    viua::internals::types::register_index const index;
    bool const iota;
    viua::internals::Register_sets const register_set;
    viua::internals::Access_specifier const access;

    Register_address(
        viua::internals::types::register_index const
        , bool const
        , viua::internals::Register_sets const
        , viua::internals::Access_specifier const
    );
};

struct Integer_literal : public Operand {
    viua::types::Integer::underlying_type const n;

    Integer_literal(viua::types::Integer::underlying_type const);
};

struct Float_literal : public Operand {
    viua::types::Float::underlying_type const n;

    Float_literal(viua::types::Float::underlying_type const);
};

struct Text_literal : public Operand {
    std::string const text;

    Text_literal(std::string);
};

struct Atom_literal : public Operand {
    std::string const text;

    Atom_literal(std::string);
};

struct Bits_literal : public Operand {
    std::string const text;

    Bits_literal(std::string);
};

struct Boolean_literal : public Operand {
    bool const value;

    Boolean_literal(bool const);
};

struct Void : public Operand {
    Void();
};

struct Function_name : public Operand {
    std::string const function_name;
    uint64_t const arity;

    Function_name(std::string, uint64_t const);
};

struct Block_name : public Operand {
    std::string const name;

    Block_name(std::string);
};

struct Module_name : public Operand {
    std::string const name;

    Module_name(std::string);
};

struct Timeout_literal : public Operand {
    std::string const value;

    Timeout_literal(std::string);
};

struct Jump_offset : public Operand {
    std::string const value;

    Jump_offset(std::string);
};

struct Jump_label : public Operand {
    std::string const value;

    Jump_label(std::string);
};

struct Instruction : public Fragment {
    OPCODE const opcode;
    std::vector<std::unique_ptr<Operand>> operands;

    Instruction(OPCODE const);
};

auto parse(std::vector<viua::tooling::libs::lexer::Token> const&) -> std::vector<std::unique_ptr<Fragment>>;

}}}}

#endif
