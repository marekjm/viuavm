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

#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/codec.h>
#include <viua/bytecode/maps.h>
#include <viua/bytecode/operand_types.h>
#include <viua/tooling/libs/lexer/tokenise.h>
#include <viua/types/float.h>
#include <viua/types/integer.h>

namespace viua { namespace tooling { namespace libs { namespace parser {
enum class Fragment_type {
    Extern_function,
    Extern_block,
    End_directive,
    Function_head,
    Closure_head,
    Block_head,
    Instruction,
    Info_directive,
    Import_directive,
    Name_directive,
    Mark_directive,
};

/*
 * Fragment represents a single element of the program; a function name,
 * an attribute list, a register address, an instruction, etc.
 * Raw tokens are reduced to fragments.
 */
class Fragment {
    std::vector<viua::tooling::libs::lexer::Token> fragment_tokens;
    Fragment_type const fragment_type;

  public:
    using Tokens_size_type = decltype(fragment_tokens)::size_type;

    auto type() const -> Fragment_type;
    auto add(viua::tooling::libs::lexer::Token) -> void;
    auto tokens() const -> decltype(fragment_tokens) const&;
    auto token(Tokens_size_type const) const
        -> viua::tooling::libs::lexer::Token const&;

    Fragment(Fragment_type const);
};

struct Extern_function : public Fragment {
    std::string const function_name;
    uint64_t const arity;

    Extern_function(std::string, uint64_t const);
};

struct Extern_block : public Fragment {
    std::string const block_name;

    Extern_block(std::string);
};

struct End_directive : public Fragment {
    End_directive();
};

struct Function_head : public Fragment {
    std::string const function_name;
    uint64_t const arity;
    std::set<std::string> const attributes;

    Function_head(std::string, uint64_t const, std::set<std::string>);
};

// FIXME Closure_head is *exactly* the same as function head, they differ only
// in fragment type
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

struct Name_directive : public Fragment {
    viua::bytecode::codec::register_index_type const register_index;
    bool const iota;
    std::string const name;

    Name_directive(viua::bytecode::codec::register_index_type const,
                   bool const,
                   std::string);
};

struct Import_directive : public Fragment {
    std::string const module_name;
    std::set<std::string> const attributes;

    Import_directive(std::string, std::set<std::string>);
};

struct Mark_directive : public Fragment {
    std::string const mark;

    Mark_directive(std::string);
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
    std::vector<viua::tooling::libs::lexer::Token> operand_tokens;
    Operand_type const operand_type;

  public:
    auto type() const -> Operand_type;
    auto add(viua::tooling::libs::lexer::Token) -> void;
    auto tokens() const -> decltype(operand_tokens) const&;

    Operand(Operand_type const);
};

struct Register_address : public Operand {
    viua::bytecode::codec::register_index_type const index;
    bool const iota;
    bool const name;
    viua::bytecode::codec::Register_set const register_set;
    viua::internals::Access_specifier const access;

    Register_address(viua::bytecode::codec::register_index_type const,
                     bool const,
                     bool const,
                     viua::bytecode::codec::Register_set const,
                     viua::internals::Access_specifier const);
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

struct Cooked_function {
    std::vector<std::unique_ptr<Fragment>> lines;

    using body_type = std::vector<Fragment const*>;
    auto head() const -> Function_head const&;
    auto body() const -> body_type;

    Cooked_function()                       = default;
    Cooked_function(Cooked_function const&) = delete;
    Cooked_function(Cooked_function&&)      = delete;
    auto operator=(Cooked_function const&) -> Cooked_function& = delete;
    auto operator=(Cooked_function &&) -> Cooked_function& = default;
};
struct Cooked_block {
    std::vector<std::unique_ptr<Fragment>> lines;

    using body_type = std::vector<Fragment const*>;
    auto body() const -> body_type;
};
struct Cooked_closure {
    std::vector<std::unique_ptr<Fragment>> lines;
};

struct Cooked_fragments {
    std::string file_name;

    std::vector<std::unique_ptr<Extern_function>> extern_function_fragments;
    std::vector<std::unique_ptr<Extern_block>> extern_block_fragments;
    std::vector<std::unique_ptr<Info_directive>> info_fragments;
    std::vector<std::unique_ptr<Import_directive>> import_fragments;

    std::map<std::string, Cooked_function> function_fragments;
    std::map<std::string, Cooked_block> block_fragments;
    std::map<std::string, Cooked_closure> closure_fragments;

    Cooked_fragments(std::string);
    Cooked_fragments(Cooked_fragments const&) = delete;
    Cooked_fragments(Cooked_fragments&&)      = default;
    auto operator=(Cooked_fragments const&) -> Cooked_fragments& = delete;
    auto operator=(Cooked_fragments &&) -> Cooked_fragments& = default;
};

auto parse(std::vector<viua::tooling::libs::lexer::Token> const&)
    -> std::vector<std::unique_ptr<Fragment>>;
auto cook(std::string, std::vector<std::unique_ptr<Fragment>>)
    -> Cooked_fragments;

}}}}  // namespace viua::tooling::libs::parser

#endif
