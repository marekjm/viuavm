/*
 *  Copyright (C) 2017 Marek Marecki
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
#include <viua/cg/lex.h>


namespace viua {
    namespace assembler {
        namespace frontend {
            namespace parser {
                enum class AccessSpecifier {
                    DIRECT,
                    REGISTER_INDIRECT,
                    POINTER_DEREFERENCE,
                };

                enum class RegisterSetSpecifier {
                    CURRENT,
                    LOCAL,
                    STATIC,
                    GLOBAL,
                };
                struct Operand {
                    std::vector<viua::cg::lex::Token> tokens;

                    auto add(viua::cg::lex::Token) -> void;
                };

                struct RegisterIndex : public Operand {
                    AccessSpecifier as;
                    viua::internals::types::register_index index;
                    RegisterSetSpecifier rss;
                };
                struct InstructionBlockName : public Operand {};
                struct BitsLiteral : public Operand {
                    std::string content;
                };
                struct IntegerLiteral : public Operand {
                    std::string content;
                };
                struct FloatLiteral : public Operand {
                    std::string content;
                };
                struct BooleanLiteral : public Operand {
                    std::string content;
                };
                struct VoidLiteral : public Operand {
                    const std::string content = "void";
                };
                struct FunctionNameLiteral : public Operand {
                    std::string content;
                };
                struct AtomLiteral : public Operand {
                    std::string content;
                };
                struct TextLiteral : public Operand {
                    std::string content;
                };
                struct DurationLiteral : public Operand {
                    std::string content;
                };
                struct Label : public Operand {
                    std::string content;
                };
                struct Offset : public Operand {
                    std::string content;
                };

                struct Line {
                    virtual ~Line() {}
                };
                struct Directive : public Line {
                    std::string directive;
                    std::vector<std::string> operands;
                };
                struct Instruction : public Line {
                    OPCODE opcode;
                    std::vector<std::unique_ptr<Operand>> operands;
                };

                struct InstructionsBlock {
                    viua::cg::lex::Token name;
                    std::map<std::string, std::string> attributes;
                    std::vector<std::unique_ptr<Line>> body;
                };

                struct ParsedSource {
                    std::vector<InstructionsBlock> functions;
                    std::vector<InstructionsBlock> blocks;
                };


                template<typename T> class vector_view {
                    const std::vector<T>& vec;
                    const typename std::remove_reference_t<decltype(vec)>::size_type offset;

                  public:
                    using size_type = decltype(offset);

                    auto at(const decltype(offset) i) const -> const T& { return vec.at(offset + i); }
                    auto size() const -> size_type { return vec.size(); }

                    vector_view(const decltype(vec) v, const decltype(offset) o) : vec(v), offset(o) {}
                    vector_view(const vector_view<T>& v, const decltype(offset) o)
                        : vec(v.vec), offset(v.offset + o) {}
                };


                auto parse_attribute_value(const vector_view<viua::cg::lex::Token> tokens, std::string&)
                    -> const decltype(tokens)::size_type;
                auto parse_attributes(const vector_view<viua::cg::lex::Token> tokens,
                                      std::map<std::string, std::string>&) -> const
                    decltype(tokens)::size_type;
                auto parse_operand(const vector_view<viua::cg::lex::Token> tokens, std::unique_ptr<Operand>&)
                    -> const decltype(tokens)::size_type;
                auto mnemonic_to_opcode(const std::string mnemonic) -> OPCODE;
                auto parse_instruction(const vector_view<viua::cg::lex::Token> tokens,
                                       std::unique_ptr<Instruction>&) -> decltype(tokens)::size_type;
                auto parse_directive(const vector_view<viua::cg::lex::Token> tokens,
                                     std::unique_ptr<Directive>&) -> decltype(tokens)::size_type;
                auto parse_line(const vector_view<viua::cg::lex::Token> tokens, std::unique_ptr<Line>&)
                    -> decltype(tokens)::size_type;
                auto parse_block_body(const vector_view<viua::cg::lex::Token> tokens, InstructionsBlock&)
                    -> decltype(tokens)::size_type;
                auto parse_function(const vector_view<viua::cg::lex::Token> tokens, InstructionsBlock&)
                    -> const decltype(tokens)::size_type;
                auto parse_block(const vector_view<viua::cg::lex::Token> tokens, InstructionsBlock&) -> const
                    decltype(tokens)::size_type;
                auto parse(const std::vector<viua::cg::lex::Token>&) -> ParsedSource;
            }
        }
    }
}


#endif
