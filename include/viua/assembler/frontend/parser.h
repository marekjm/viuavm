/*
 *  Copyright (C) 2017, 2018 Marek Marecki
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

#ifndef VIUA_ASSEMBLER_FRONTEND_PARSER_H
#define VIUA_ASSEMBLER_FRONTEND_PARSER_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/operand_types.h>
#include <viua/cg/lex.h>
#include <viua/front/asm.h>


namespace viua { namespace assembler { namespace frontend {
auto gather_functions(std::vector<viua::cg::lex::Token> const&)
    -> viua::front::assembler::Invocables;
auto gather_blocks(std::vector<viua::cg::lex::Token> const& tokens)
    -> viua::front::assembler::Invocables;
auto gather_meta_information(std::vector<viua::cg::lex::Token> const& tokens)
    -> std::map<std::string, std::string>;

namespace parser {
struct Operand {
    std::vector<viua::cg::lex::Token> tokens;
    std::map<std::string, std::string> attributes;

    auto add(viua::cg::lex::Token) -> void;

    Operand()               = default;
    Operand(Operand const&) = default;
    virtual ~Operand()      = default;
};

struct Register_index : public Operand {
    viua::internals::Access_specifier as;
    viua::bytecode::codec::register_index_type index;
    viua::internals::Register_sets rss;
    bool resolved = false;
};
struct Instruction_block_name : public Operand {
};
struct Bits_literal : public Operand {
    std::string content;
};
struct Integer_literal : public Operand {
    std::string content;
};
struct Float_literal : public Operand {
    std::string content;
};
struct Boolean_literal : public Operand {
    std::string content;
};
struct Void_literal : public Operand {
    std::string const content = "void";
};
struct Function_name_literal : public Operand {
    std::string content;
};
struct Atom_literal : public Operand {
    std::string content;
};
struct Text_literal : public Operand {
    std::string content;
};
struct Duration_literal : public Operand {
    std::string content;
};
struct Label : public Operand {
    std::string content;
};
struct Offset : public Operand {
    std::string content;
};

struct Line {
    std::vector<viua::cg::lex::Token> tokens;

    auto add(viua::cg::lex::Token) -> void;

    virtual ~Line() = default;
};
struct Directive : public Line {
    std::string directive;
    std::vector<std::string> operands;
};
struct Instruction : public Line {
    OPCODE opcode;
    std::vector<std::unique_ptr<Operand>> operands;
};

struct Instructions_block {
    viua::cg::lex::Token name;
    viua::cg::lex::Token ending_token;
    std::map<std::string, std::string> attributes;
    std::vector<std::unique_ptr<Line>> body;
    bool closure = false;

    using size_type = decltype(body)::size_type;

    std::map<std::string, size_type> marker_map;
};

struct Parsed_source {
    std::vector<Instructions_block> functions;
    std::vector<viua::cg::lex::Token> function_signatures;

    std::vector<Instructions_block> blocks;
    std::vector<viua::cg::lex::Token> block_signatures;

    std::map<std::string, std::map<std::string, std::string>> imports;

    bool as_library = false;

    auto block(std::string const) const -> Instructions_block const&;
};


template<typename T> class vector_view {
    std::vector<T> const& vec;
    const typename std::remove_reference_t<decltype(vec)>::size_type offset;

  public:
    using size_type = std::remove_const_t<decltype(offset)>;

    auto at(decltype(offset) const i) const -> T const&
    {
        return vec.at(offset + i);
    }
    auto size() const -> size_type { return vec.size(); }

    vector_view(const decltype(vec) v, const decltype(offset) o)
            : vec(v), offset(o)
    {}
    vector_view(const vector_view<T>& v, const decltype(offset) o)
            : vec(v.vec), offset(v.offset + o)
    {}
};

auto parse_attribute_value(const vector_view<viua::cg::lex::Token> tokens,
                           std::string&) -> decltype(tokens)::size_type;
auto parse_attributes(const vector_view<viua::cg::lex::Token> tokens,
                      std::map<std::string, std::string>&)
    -> decltype(tokens)::size_type;
auto parse_operand(const vector_view<viua::cg::lex::Token> tokens,
                   std::unique_ptr<Operand>&,
                   bool const = false) -> decltype(tokens)::size_type;
auto mnemonic_to_opcode(std::string const mnemonic) -> OPCODE;
auto parse_instruction(const vector_view<viua::cg::lex::Token> tokens,
                       std::unique_ptr<Instruction>&)
    -> decltype(tokens)::size_type;
auto parse_directive(const vector_view<viua::cg::lex::Token> tokens,
                     std::unique_ptr<Directive>&)
    -> decltype(tokens)::size_type;
auto parse_line(const vector_view<viua::cg::lex::Token> tokens,
                std::unique_ptr<Line>&) -> decltype(tokens)::size_type;
auto parse_block_body(const vector_view<viua::cg::lex::Token> tokens,
                      Instructions_block&) -> decltype(tokens)::size_type;
auto parse_function(const vector_view<viua::cg::lex::Token> tokens,
                    Instructions_block&) -> decltype(tokens)::size_type;
auto parse_closure(const vector_view<viua::cg::lex::Token> tokens,
                   Instructions_block&) -> decltype(tokens)::size_type;
auto parse_block(const vector_view<viua::cg::lex::Token> tokens,
                 Instructions_block&) -> decltype(tokens)::size_type;
auto parse(std::vector<viua::cg::lex::Token> const&) -> Parsed_source;
}  // namespace parser
}}}  // namespace viua::assembler::frontend


#endif
