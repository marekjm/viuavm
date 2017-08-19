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

#include <iostream>
#include <viua/assembler/frontend/parser.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/support/string.h>
using namespace std;


using Token = viua::cg::lex::Token;
using InvalidSyntax = viua::cg::lex::InvalidSyntax;


auto viua::assembler::frontend::parser::Operand::add(viua::cg::lex::Token t) -> void { tokens.push_back(t); }

auto viua::assembler::frontend::parser::Line::add(viua::cg::lex::Token t) -> void { tokens.push_back(t); }

auto viua::assembler::frontend::parser::parse_attribute_value(const vector_view<Token> tokens, string& value)
    -> decltype(tokens)::size_type {
    auto i = decltype(tokens)::size_type{1};

    if (tokens.at(i) == "}") {
        // do nothing
    } else {
        value = tokens.at(i).str();
        ++i;
    }

    if (tokens.at(i) != "}") {
        throw viua::cg::lex::InvalidSyntax(tokens.at(i), "expected '}'");
    }
    ++i;

    return i;
}
auto viua::assembler::frontend::parser::parse_attributes(const vector_view<Token> tokens,
                                                         decltype(InstructionsBlock::attributes) & attributes)
    -> decltype(tokens)::size_type {
    auto i = decltype(tokens)::size_type{0};

    if (tokens.at(i) != "[[") {
        throw InvalidSyntax(tokens.at(i), "expected '[['");
    }
    ++i;  // skip '[['

    while (i < tokens.size() and tokens.at(i) != "]]") {
        const string key = tokens.at(i++);
        string value;

        if (tokens.at(i) == ",") {
            ++i;
        } else if (tokens.at(i) == "{") {
            i += parse_attribute_value(vector_view<Token>(tokens, i), value);
        } else if (tokens.at(i) == "]]") {
            // do nothing
        } else {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), "expected ',' or '{' or ']]'");
        }

        cerr << "  attribute: " << key << '{' << value << '}' << endl;
        attributes[key] = value;
    }
    ++i;  // skip ']]'

    if (i == tokens.size()) {
        throw InvalidSyntax(tokens.at(i - 1), "unexpected end-of-file: expected function name");
    }

    return i;
}

auto viua::assembler::frontend::parser::parse_operand(const vector_view<Token> tokens,
                                                      unique_ptr<Operand>& operand)
    -> decltype(tokens)::size_type {
    auto i = std::remove_reference_t<decltype(tokens)>::size_type{0};

    auto tok = tokens.at(i).str();
    cerr << "    operand: " << tok << endl;

    if (tok.at(0) == '%' or tok.at(0) == '*' or tok.at(0) == '@') {
        auto ri = make_unique<RegisterIndex>();

        if (tok.at(0) == '%') {
            ri->as = AccessSpecifier::DIRECT;
        } else if (tok.at(0) == '*') {
            ri->as = AccessSpecifier::POINTER_DEREFERENCE;
        } else if (tok.at(0) == '@') {
            ri->as = AccessSpecifier::REGISTER_INDIRECT;
        }
        ri->add(tokens.at(i));  // add index token

        ri->index = static_cast<decltype(ri->index)>(stoul(tok.substr(1)));
        ++i;

        auto has_rss = true;
        if (tokens.at(i) == "current") {
            ri->rss = RegisterSetSpecifier::CURRENT;
        } else if (tokens.at(i) == "local") {
            ri->rss = RegisterSetSpecifier::LOCAL;
        } else if (tokens.at(i) == "static") {
            ri->rss = RegisterSetSpecifier::STATIC;
        } else if (tokens.at(i) == "global") {
            ri->rss = RegisterSetSpecifier::GLOBAL;
        } else {
            /*
             * This is just for 'arg' instruction's special-case, where
             * the second operand *is* preceded by a '%' character, but
             * *is not* followed by a register set specifier.
             *
             * FIXME Add 'arguments' register set specifier (read only) to
             * un-special-case the 'arg' instruction.
             */
            has_rss = false;
        }

        if (has_rss) {
            ri->add(tokens.at(i));  // add register set specifier token
            ++i;
        }

        operand = std::move(ri);
    } else if (str::is_binary_literal(tok)) {
        auto bits_literal = make_unique<BitsLiteral>();
        bits_literal->content = tokens.at(i);
        bits_literal->add(tokens.at(i));
        ++i;

        operand = std::move(bits_literal);
    } else if (str::isnum(tok, true)) {
        auto integer_literal = make_unique<IntegerLiteral>();
        integer_literal->content = tokens.at(i);
        integer_literal->add(tokens.at(i));
        ++i;

        operand = std::move(integer_literal);
    } else if (str::isfloat(tok, true)) {
        auto float_literal = make_unique<FloatLiteral>();
        float_literal->content = tokens.at(i);
        float_literal->add(tokens.at(i));
        ++i;

        operand = std::move(float_literal);
    } else if (str::is_boolean_literal(tok)) {
        auto boolean_literal = make_unique<BooleanLiteral>();
        boolean_literal->content = tokens.at(i);
        boolean_literal->add(tokens.at(i));
        ++i;

        operand = std::move(boolean_literal);
    } else if (str::is_void(tok)) {
        auto void_literal = make_unique<VoidLiteral>();
        void_literal->add(tokens.at(i));
        ++i;

        operand = std::move(void_literal);
    } else if (str::isid(tok) and not OP_MNEMONICS.count(tok)) {
        auto label = make_unique<Label>();
        label->content = tokens.at(i);
        label->add(tokens.at(i));
        ++i;

        operand = std::move(label);
    } else if (::assembler::utils::isValidFunctionName(tok)) {
        auto fn_name_literal = make_unique<FunctionNameLiteral>();
        fn_name_literal->content = tokens.at(i);
        fn_name_literal->add(tokens.at(i));
        ++i;

        operand = std::move(fn_name_literal);
    } else if (str::is_atom_literal(tok)) {
        auto atom_literal = make_unique<AtomLiteral>();
        atom_literal->content = tokens.at(i);
        atom_literal->add(tokens.at(i));
        ++i;

        operand = std::move(atom_literal);
    } else if (str::is_text_literal(tok)) {
        auto text_literal = make_unique<TextLiteral>();
        text_literal->content = tokens.at(i);
        text_literal->add(tokens.at(i));
        ++i;

        operand = std::move(text_literal);
    } else if (str::is_timeout_literal(tok)) {
        auto duration_literal = make_unique<DurationLiteral>();
        duration_literal->content = tokens.at(i);
        duration_literal->add(tokens.at(i));
        ++i;

        operand = std::move(duration_literal);
    } else if (tok.at(0) == '+' and str::isnum(tok.substr(1))) {
        auto offset = make_unique<Offset>();
        offset->content = tokens.at(i);
        offset->add(tokens.at(i));
        ++i;

        operand = std::move(offset);
    } else {
        throw viua::cg::lex::InvalidSyntax(tokens.at(i), "invalid operand");
    }

    return i;
}

auto viua::assembler::frontend::parser::mnemonic_to_opcode(const string mnemonic) -> OPCODE {
    OPCODE opcode = NOP;
    for (const auto each : OP_NAMES) {
        if (each.second == mnemonic) {
            opcode = each.first;
            break;
        }
    }
    return opcode;
}
auto viua::assembler::frontend::parser::parse_instruction(const vector_view<Token> tokens,
                                                          unique_ptr<Instruction>& instruction)
    -> decltype(tokens)::size_type {
    auto i = decltype(tokens)::size_type{0};

    if (not OP_MNEMONICS.count(tokens.at(i).str())) {
        throw viua::cg::lex::InvalidSyntax(tokens.at(i), "expected mnemonic");
    }

    cerr << "  mnemonic: " << tokens.at(i).str() << endl;
    instruction->opcode = mnemonic_to_opcode(tokens.at(i++).str());

    try {
        while (tokens.at(i) != "\n") {
            unique_ptr<Operand> operand;
            i += parse_operand(vector_view<Token>(tokens, i), operand);
            instruction->operands.push_back(std::move(operand));
        }
        ++i;  // skip newline
    } catch (viua::cg::lex::InvalidSyntax& e) { throw e.add(tokens.at(0)); }

    return i;
}
auto viua::assembler::frontend::parser::parse_directive(const vector_view<Token> tokens,
                                                        unique_ptr<Directive>& directive)
    -> decltype(tokens)::size_type {
    auto i = decltype(tokens)::size_type{0};

    if (not::assembler::utils::lines::is_directive(tokens.at(0))) {
        throw viua::cg::lex::InvalidSyntax(tokens.at(0), "unknown directive");
    }

    if (tokens.at(0) == ".block:" or tokens.at(0) == ".function:" or tokens.at(0) == ".closure:") {
        throw viua::cg::lex::InvalidSyntax(tokens.at(0), "no '.end' between definitions");
    }

    cerr << "  directive: " << tokens.at(i).str() << endl;
    directive->directive = tokens.at(i++);

    if (tokens.at(0) == ".iota:") {
        if (not str::isnum(tokens.at(i), false)) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), "expected a positive integer");
        }
        directive->operands.push_back(tokens.at(i++));
    } else if (tokens.at(0) == ".mark:") {
        if (not str::isid(tokens.at(i))) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), "invalid marker");
        }
        directive->operands.push_back(tokens.at(i++));
    } else if (tokens.at(0) == ".unused:") {
        directive->operands.push_back(tokens.at(i++));
    }

    if (tokens.at(i) != "\n") {
        throw viua::cg::lex::InvalidSyntax(tokens.at(i), "extra parameters to a directive").add(tokens.at(0));
    }
    ++i;  // skip newline

    return i;
}
auto viua::assembler::frontend::parser::parse_line(const vector_view<Token> tokens, unique_ptr<Line>& line)
    -> decltype(tokens)::size_type {
    auto i = decltype(tokens)::size_type{0};
    if (tokens.at(0).str().at(0) == '.') {
        auto directive = make_unique<Directive>();
        i = parse_directive(vector_view<Token>(tokens, 0), directive);
        line = std::move(directive);
    } else {
        auto instruction = make_unique<Instruction>();
        i = parse_instruction(vector_view<Token>(tokens, 0), instruction);
        line = std::move(instruction);
    }

    for (auto j = decltype(i){0}; j < i; ++j) {
        line->add(tokens.at(j));
    }

    return i;
}

auto viua::assembler::frontend::parser::parse_block_body(const vector_view<Token> tokens,
                                                         InstructionsBlock& instructions_block)
    -> decltype(tokens)::size_type {
    auto i = std::remove_reference_t<decltype(tokens)>::size_type{0};

    while (tokens.at(i) != ".end") {
        auto line = make_unique<Line>();
        i += parse_line(vector_view<Token>(tokens, i), line);
        instructions_block.body.push_back(std::move(line));
    }
    if (tokens.at(i) != ".end") {
        throw InvalidSyntax(tokens.at(i), "no '.end' at the end of a block");
    }
    instructions_block.ending_token = tokens.at(i);

    return i;
}

auto viua::assembler::frontend::parser::parse_function(const vector_view<Token> tokens, InstructionsBlock& ib)
    -> decltype(tokens)::size_type {
    auto i = std::remove_reference_t<decltype(tokens)>::size_type{1};

    cerr << "parsing function" << endl;

    i += parse_attributes(vector_view<Token>(tokens, i), ib.attributes);

    cerr << "  name: " << tokens.at(i).str() << endl;
    ib.name = tokens.at(i);

    if (not::assembler::utils::isValidFunctionName(ib.name)) {
        throw InvalidSyntax(ib.name, ("invalid function name: " + ib.name.str()));
    }
    if (::assembler::utils::getFunctionArity(ib.name) == -1) {
        throw InvalidSyntax(ib.name, ("function with undefined arity: " + ib.name.str()));
    }

    ++i;  // skip name

    if (tokens.at(i) != "\n") {
        throw InvalidSyntax(tokens.at(i), "unexpected token after function name");
    }
    ++i;  // skip newline

    i += parse_block_body(vector_view<Token>(tokens, i), ib);
    if (not ib.body.size()) {
        throw InvalidSyntax(ib.name, ("function with empty body: " + ib.name.str()));
    }

    return i;
}
auto viua::assembler::frontend::parser::parse_closure(const vector_view<Token> tokens, InstructionsBlock& ib)
    -> decltype(tokens)::size_type {
    return parse_function(tokens, ib);
}
auto viua::assembler::frontend::parser::parse_block(const vector_view<Token> tokens, InstructionsBlock& ib)
    -> decltype(tokens)::size_type {
    auto i = std::remove_reference_t<decltype(tokens)>::size_type{1};

    cerr << "parsing block" << endl;

    i += parse_attributes(vector_view<Token>(tokens, i), ib.attributes);

    cerr << "  name: " << tokens.at(i).str() << endl;
    ib.name = tokens.at(i);

    ++i;  // skip name

    if (tokens.at(i) != "\n") {
        throw InvalidSyntax(tokens.at(i), "unexpected token after block name");
    }
    ++i;  // skip newline

    i += parse_block_body(vector_view<Token>(tokens, i), ib);
    if (not ib.body.size()) {
        throw InvalidSyntax(ib.name, ("block with empty body: " + ib.name.str()));
    }

    return i;
}

auto viua::assembler::frontend::parser::parse(const vector<Token>& tokens) -> ParsedSource {
    ParsedSource parsed;

    for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{0}; i < tokens.size(); ++i) {
        if (tokens.at(i) == "\n") {
            continue;
        }
        if (tokens.at(i) == ".function:") {
            InstructionsBlock ib;
            i += parse_function(vector_view<Token>(tokens, i), ib);
            parsed.functions.push_back(std::move(ib));
        } else if (tokens.at(i) == ".block:") {
            InstructionsBlock ib;
            i += parse_block(vector_view<Token>(tokens, i), ib);
        } else if (tokens.at(i) == ".closure:") {
            InstructionsBlock ib;
            i += parse_closure(vector_view<Token>(tokens, i), ib);
        } else if (tokens.at(i) == ".signature:") {
            ++i;
            if (tokens.at(i) == "\n") {
                throw InvalidSyntax(tokens.at(i - 1), "missing function name");
            }
            if (not::assembler::utils::isValidFunctionName(tokens.at(i))) {
                throw InvalidSyntax(tokens.at(i), "not a valid function name");
            }
            parsed.function_signatures.push_back(tokens.at(i++));
        } else if (tokens.at(i) == ".bsignature:") {
            ++i;
            if (tokens.at(i) == "\n") {
                throw InvalidSyntax(tokens.at(i - 1), "missing block name");
            }
            parsed.block_signatures.push_back(tokens.at(i++));
        } else {
            throw viua::cg::lex::InvalidSyntax(
                tokens.at(i), "expected a function or a block definition (or signature), or a newline");
        }
    }

    return parsed;
}
