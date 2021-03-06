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

#include <iostream>
#include <stdexcept>
#include <limits>

#include <viua/assembler/frontend/parser.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/support/string.h>


using viua::bytecode::codec::Register_set;
using viua::cg::lex::Invalid_syntax;
using viua::cg::lex::Token;
using viua::cg::lex::Traced_syntax_error;
using viua::internals::Access_specifier;


// This value is completely arbitrary.
const auto max_distance_for_misspelled_ids = str::LevenshteinDistance{4};


auto viua::assembler::frontend::parser::Parsed_source::block(
    std::string const name) const -> Instructions_block const&
{
    for (auto const& each : blocks) {
        if (each.name == name) {
            return each;
        }
    }
    throw std::out_of_range(name);
}


auto viua::assembler::frontend::parser::Operand::add(Token t) -> void
{
    tokens.push_back(t);
}

auto viua::assembler::frontend::parser::Line::add(Token t) -> void
{
    tokens.push_back(t);
}

auto viua::assembler::frontend::parser::parse_attribute_value(
    const vector_view<Token> tokens,
    std::string& value) -> decltype(tokens)::size_type
{
    auto i = decltype(tokens)::size_type{1};

    if (tokens.at(i) == "}") {
        // do nothing
    } else {
        value = tokens.at(i).str();
        ++i;
    }

    if (tokens.at(i) != "}") {
        throw Invalid_syntax(tokens.at(i), "expected '}'");
    }
    ++i;

    return i;
}
auto viua::assembler::frontend::parser::parse_attributes(
    const vector_view<Token> tokens,
    decltype(Instructions_block::attributes)& attributes)
    -> decltype(tokens)::size_type
{
    auto i = decltype(tokens)::size_type{0};

    if (tokens.at(i) != "[[") {
        throw Invalid_syntax(tokens.at(i), "expected '[['");
    }
    ++i;  // skip '[['

    while (i < tokens.size() and tokens.at(i) != "]]") {
        std::string const key = tokens.at(i++);
        auto value            = std::string{};

        if (tokens.at(i) == ",") {
            ++i;
        } else if (tokens.at(i) == "{") {
            i += parse_attribute_value(vector_view<Token>(tokens, i), value);
        } else if (tokens.at(i) == "]]") {
            // do nothing
        } else {
            throw Invalid_syntax(tokens.at(i), "expected ',' or '{' or ']]'");
        }

        attributes[key] = value;
    }
    ++i;  // skip ']]'

    if (i == tokens.size()) {
        throw Invalid_syntax(tokens.at(i - 1),
                             "unexpected end-of-file: expected function name");
    }

    return i;
}

auto viua::assembler::frontend::parser::parse_operand(
    const vector_view<Token> tokens,
    std::unique_ptr<Operand>& operand,
    bool const integer_literal_means_offset) -> decltype(tokens)::size_type
{
    auto i = std::remove_reference_t<decltype(tokens)>::size_type{0};

    auto tok = tokens.at(i).str();

    if (tok.at(0) == '%' or tok.at(0) == '*' or tok.at(0) == '@') {
        auto ri = std::make_unique<Register_index>();

        if (tok.at(0) == '%') {
            ri->as = Access_specifier::DIRECT;
        } else if (tok.at(0) == '*') {
            ri->as = Access_specifier::POINTER_DEREFERENCE;
        } else if (tok.at(0) == '@') {
            ri->as = Access_specifier::REGISTER_INDIRECT;
        }
        ri->add(tokens.at(i));  // add index token

        if (str::isnum(tok.substr(1), false)) {
            constexpr auto max_allowed = std::numeric_limits<
                viua::bytecode::codec::register_index_type>::max();
            try {
                /*
                 * We check the string value here since it is not susceptible to
                 * wraparound. If we checked the index value we would access a
                 * value that was potentially tainted by wraparound - which
                 * could lead to a false sense of security here, and weird error
                 * messages like "cannot access register 0 with 0 allocated"
                 * while the underlined source clearly states 65536.
                 *
                 * Let's use std::stoull() to get an uint64_t. The range on this
                 * type is sufficiently large that it would be unusual to exceed
                 * it.
                 */
                auto const actual_tried = std::stoull(tok.substr(1));
                if (actual_tried > max_allowed) {
                    throw std::out_of_range{
                        "tried to allocate too many registers"};
                }
                ri->index = static_cast<decltype(ri->index)>(actual_tried);
            } catch (std::out_of_range&) {
                throw Invalid_syntax{
                    tokens.at(0),
                    "register index outside of defined range (max allowed "
                    "register index is "
                        + std::to_string(max_allowed) + ')'};
            }
            ri->resolved = true;
        } else if (str::isnum(tok.substr(1), true)) {
            throw Invalid_syntax(
                tokens.at(0),
                "register indexes cannot be negative: " + tok.substr(1));
        } else {
            // FIXME Throw this error during register usage analysis, when we
            // have a full map of names built so "did you mean...?" note can be
            // provided. Mark the register index as unresolved to prevent it
            // from being accidentally used. throw Invalid_syntax(tokens.at(0),
            // "undeclared register name: " + tok.substr(1));
            ri->resolved = false;
        }

        ++i;

        auto has_rss = true;
        if (tokens.at(i) == "current") {
            throw Invalid_syntax{tokens.at(i),
                                 "current register set is illegal"};
        } else if (tokens.at(i) == "local") {
            ri->rss = viua::bytecode::codec::Register_set::Local;
        } else if (tokens.at(i) == "static") {
            ri->rss = viua::bytecode::codec::Register_set::Static;
        } else if (tokens.at(i) == "global") {
            ri->rss = viua::bytecode::codec::Register_set::Global;
        } else if (tokens.at(i) == "parameters") {
            ri->rss = viua::bytecode::codec::Register_set::Parameters;
        } else if (tokens.at(i) == "arguments") {
            ri->rss = viua::bytecode::codec::Register_set::Arguments;
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
        auto bits_literal     = std::make_unique<Bits_literal>();
        bits_literal->content = tokens.at(i);
        bits_literal->add(tokens.at(i));
        ++i;

        operand = std::move(bits_literal);
    } else if (str::isnum(tok, true) and not integer_literal_means_offset) {
        auto integer_literal     = std::make_unique<Integer_literal>();
        integer_literal->content = tokens.at(i);
        integer_literal->add(tokens.at(i));
        ++i;

        operand = std::move(integer_literal);
    } else if (str::isfloat(tok, true)) {
        auto float_literal     = std::make_unique<Float_literal>();
        float_literal->content = tokens.at(i);
        float_literal->add(tokens.at(i));
        ++i;

        operand = std::move(float_literal);
    } else if (str::is_boolean_literal(tok)) {
        auto boolean_literal     = std::make_unique<Boolean_literal>();
        boolean_literal->content = tokens.at(i);
        boolean_literal->add(tokens.at(i));
        ++i;

        operand = std::move(boolean_literal);
    } else if (str::is_void(tok)) {
        auto void_literal = std::make_unique<Void_literal>();
        void_literal->add(tokens.at(i));
        ++i;

        operand = std::move(void_literal);
    } else if (str::is_register_set_name(tok)) {
        auto label = std::make_unique<Label>();  // FIXME use a special type for
                                                 // register set names, not the
                                                 // 'Label' type - register set
                                                 // names are not really labels
        label->content = tokens.at(i);
        label->add(tokens.at(i));
        ++i;

        operand = std::move(label);
    } else if (::assembler::utils::is_valid_function_name(tok)) {
        auto fn_name_literal     = std::make_unique<Function_name_literal>();
        fn_name_literal->content = tokens.at(i);
        fn_name_literal->add(tokens.at(i));
        ++i;

        operand = std::move(fn_name_literal);
    } else if (str::isid(tok) and not viua::cg::lex::is_mnemonic(tok)) {
        auto label     = std::make_unique<Label>();
        label->content = tokens.at(i);
        label->add(tokens.at(i));
        ++i;

        operand = std::move(label);
    } else if (str::is_atom_literal(tok)) {
        auto atom_literal     = std::make_unique<Atom_literal>();
        atom_literal->content = tokens.at(i);
        atom_literal->add(tokens.at(i));
        ++i;

        operand = std::move(atom_literal);
    } else if (str::is_text_literal(tok)) {
        auto text_literal     = std::make_unique<Text_literal>();
        text_literal->content = tokens.at(i);
        text_literal->add(tokens.at(i));
        ++i;

        operand = std::move(text_literal);
    } else if (str::is_timeout_literal(tok)) {
        auto duration_literal     = std::make_unique<Duration_literal>();
        duration_literal->content = tokens.at(i);
        duration_literal->add(tokens.at(i));
        ++i;

        operand = std::move(duration_literal);
    } else if ((tok.at(0) == '+' and str::isnum(tok.substr(1)))
               or str::isnum(tok, true)) {
        auto offset     = std::make_unique<Offset>();
        offset->content = (tok.at(0) == '+' ? tok.substr(1) : tok);
        offset->add(tokens.at(i));
        ++i;

        operand = std::move(offset);
    } else {
        throw Invalid_syntax(tokens.at(i), "invalid operand");
    }

    if (tokens.at(i) == "[[") {
        i += parse_attributes(vector_view<Token>(tokens, i),
                              operand->attributes);
    }

    return i;
}

auto viua::assembler::frontend::parser::mnemonic_to_opcode(
    std::string const mnemonic) -> OPCODE
{
    OPCODE opcode = NOP;
    for (auto const& each : OP_NAMES) {
        if (each.second == mnemonic) {
            opcode = each.first;
            break;
        }
    }
    return opcode;
}
static auto get_mnemonics() -> std::vector<std::string>
{
    auto mnemonics = std::vector<std::string>{};
    for (auto const& each : OP_NAMES) {
        mnemonics.push_back(std::move(each.second));
    }
    return mnemonics;
}
static auto get_directives() -> std::vector<std::string>
{
    return {
        ".end",
        ".function:",
        ".closure:",
        ".block:",
        ".iota:",
        ".mark:",
        ".name:",
        ".unused:",
        ".import:",
    };
}
auto viua::assembler::frontend::parser::parse_instruction(
    const vector_view<Token> tokens,
    std::unique_ptr<Instruction>& instruction) -> decltype(tokens)::size_type
{
    auto i = decltype(tokens)::size_type{0};

    if (tokens.at(i).str().at(0) == '.') {
        throw Invalid_syntax(tokens.at(i), "expected mnemonic");
    }
    if (viua::cg::lex::is_register_set_name(tokens.at(i).str())) {
        auto error = Invalid_syntax(
            tokens.at(i),
            "register set specifier does not follow register index");
        throw error;
    }
    if (not viua::cg::lex::is_mnemonic(tokens.at(i).str())) {
        auto error = Invalid_syntax(tokens.at(i), "unknown instruction");
        if (auto suggestion = str::levenshtein_best(
                tokens.at(i), get_mnemonics(), max_distance_for_misspelled_ids);
            suggestion.first) {
            error.aside(tokens.at(i),
                        "did you mean '" + suggestion.second + "'?");
        }
        throw error;
    }

    instruction->opcode = mnemonic_to_opcode(tokens.at(i++).str());
    bool integer_literal_means_offset =
        (instruction->opcode == JUMP or instruction->opcode == IF);

    try {
        while (tokens.at(i) != "\n") {
            std::unique_ptr<Operand> operand;
            i += parse_operand(vector_view<Token>(tokens, i),
                               operand,
                               integer_literal_means_offset);
            instruction->operands.push_back(std::move(operand));
        }
        ++i;  // skip newline
    } catch (Invalid_syntax& e) {
        throw e.add(tokens.at(0));
    }

    return i;
}
auto viua::assembler::frontend::parser::parse_directive(
    const vector_view<Token> tokens,
    std::unique_ptr<Directive>& directive) -> decltype(tokens)::size_type
{
    auto i = decltype(tokens)::size_type{0};

    if (not ::assembler::utils::lines::is_directive(tokens.at(0))) {
        auto error = Invalid_syntax(tokens.at(0), "unknown directive");
        if (auto suggestion =
                str::levenshtein_best(tokens.at(i),
                                      get_directives(),
                                      max_distance_for_misspelled_ids);
            suggestion.first) {
            error.aside("did you mean '" + suggestion.second + "'?");
        }
        throw error;
    }

    if (tokens.at(0) == ".block:" or tokens.at(0) == ".function:"
        or tokens.at(0) == ".closure:") {
        throw Invalid_syntax(tokens.at(0), "no '.end' between definitions");
    }

    directive->directive = tokens.at(i++);

    if (tokens.at(0) == ".iota:") {
        if (not str::isnum(tokens.at(i), false)) {
            throw Invalid_syntax(tokens.at(i), "expected a positive integer");
        }
        directive->operands.push_back(tokens.at(i++));
    } else if (tokens.at(0) == ".mark:") {
        if (not str::isid(tokens.at(i))) {
            throw Invalid_syntax(tokens.at(i), "invalid marker");
        }
        directive->operands.push_back(tokens.at(i++));
    } else if (tokens.at(0) == ".unused:") {
        directive->operands.push_back(tokens.at(i++));
    } else if (tokens.at(0) == ".name:") {
        directive->operands.push_back(tokens.at(i++));
        directive->operands.push_back(tokens.at(i++));
    }

    if (tokens.at(i) != "\n") {
        throw Invalid_syntax(tokens.at(i), "extra parameters to a directive")
            .add(tokens.at(0));
    }
    ++i;  // skip newline

    return i;
}
auto viua::assembler::frontend::parser::parse_line(
    const vector_view<Token> tokens,
    std::unique_ptr<Line>& line) -> decltype(tokens)::size_type
{
    auto i = decltype(tokens)::size_type{0};
    if (tokens.at(0).str().at(0) == '.') {
        auto directive = std::make_unique<Directive>();
        i    = parse_directive(vector_view<Token>(tokens, 0), directive);
        line = std::move(directive);
    } else {
        auto instruction = std::make_unique<Instruction>();
        i    = parse_instruction(vector_view<Token>(tokens, 0), instruction);
        line = std::move(instruction);
    }

    for (auto j = decltype(i){0}; j < i; ++j) {
        line->add(tokens.at(j));
    }

    return i;
}

using viua::assembler::frontend::parser::Directive;
using viua::assembler::frontend::parser::Instructions_block;
using InstructionIndex = decltype(
    viua::assembler::frontend::parser::Instructions_block::body)::size_type;
static auto populate_marker_map(Instructions_block& instructions_block) -> void
{
    // XXX HACK start from maximum value, and wrap to zero when
    // incremented for first instruction; this is a hack
    auto instruction_counter = static_cast<InstructionIndex>(-1);
    for (auto const& line : instructions_block.body) {
        if (const auto directive = dynamic_cast<Directive*>(line.get());
            directive) {
            if (directive->directive == ".mark:") {
                instructions_block.marker_map[directive->operands.at(0)] =
                    instruction_counter + 1;
            }
        } else {
            ++instruction_counter;
        }
    }
}
auto viua::assembler::frontend::parser::parse_block_body(
    const vector_view<Token> tokens,
    Instructions_block& instructions_block) -> decltype(tokens)::size_type
{
    auto i = std::remove_reference_t<decltype(tokens)>::size_type{0};

    while (tokens.at(i) != ".end") {
        auto line = std::make_unique<Line>();
        i += parse_line(vector_view<Token>(tokens, i), line);
        instructions_block.body.push_back(std::move(line));
    }
    if (tokens.at(i) != ".end") {
        throw Invalid_syntax(tokens.at(i), "no '.end' at the end of a block");
    }
    instructions_block.ending_token = tokens.at(i);

    populate_marker_map(instructions_block);

    return i;
}

auto viua::assembler::frontend::parser::parse_function(
    const vector_view<Token> tokens,
    Instructions_block& ib) -> decltype(tokens)::size_type
{
    auto i = std::remove_reference_t<decltype(tokens)>::size_type{1};

    i += parse_attributes(vector_view<Token>(tokens, i), ib.attributes);

    ib.name = tokens.at(i);

    if (not ::assembler::utils::is_valid_function_name(ib.name)) {
        throw Invalid_syntax(ib.name,
                             ("invalid function name: " + ib.name.str()));
    }

    ++i;  // skip name

    if (tokens.at(i) != "\n") {
        throw Invalid_syntax(tokens.at(i),
                             "unexpected token after function name");
    }
    ++i;  // skip newline

    try {
        i += parse_block_body(vector_view<Token>(tokens, i), ib);
    } catch (Invalid_syntax& e) {
        throw Traced_syntax_error().append(e).append(
            Invalid_syntax(ib.name, ("in function " + ib.name.str())));
    }

    if (not ib.body.size()) {
        throw Invalid_syntax(ib.name,
                             ("function with empty body: " + ib.name.str()));
    }

    return i;
}
auto viua::assembler::frontend::parser::parse_closure(
    const vector_view<Token> tokens,
    Instructions_block& ib) -> decltype(tokens)::size_type
{
    auto i = std::remove_reference_t<decltype(tokens)>::size_type{1};

    i += parse_attributes(vector_view<Token>(tokens, i), ib.attributes);

    ib.closure = true;
    ib.name    = tokens.at(i);

    if (not ::assembler::utils::is_valid_function_name(ib.name)) {
        throw Invalid_syntax(ib.name,
                             ("invalid function name: " + ib.name.str()));
    }
    if (::assembler::utils::get_function_arity(ib.name) == -1) {
        throw Invalid_syntax(
            ib.name, ("function with undefined arity: " + ib.name.str()));
    }

    ++i;  // skip name

    if (tokens.at(i) != "\n") {
        throw Invalid_syntax(tokens.at(i),
                             "unexpected token after function name");
    }
    ++i;  // skip newline

    try {
        i += parse_block_body(vector_view<Token>(tokens, i), ib);
    } catch (Invalid_syntax& e) {
        throw Traced_syntax_error().append(e).append(
            Invalid_syntax(ib.name, ("in function " + ib.name.str())));
    }

    if (not ib.body.size()) {
        throw Invalid_syntax(ib.name,
                             ("function with empty body: " + ib.name.str()));
    }

    return i;
}
auto viua::assembler::frontend::parser::parse_block(
    const vector_view<Token> tokens,
    Instructions_block& ib) -> decltype(tokens)::size_type
{
    auto i = std::remove_reference_t<decltype(tokens)>::size_type{1};

    i += parse_attributes(vector_view<Token>(tokens, i), ib.attributes);

    ib.name = tokens.at(i);

    ++i;  // skip name

    if (tokens.at(i) != "\n") {
        throw Invalid_syntax(tokens.at(i), "unexpected token after block name");
    }
    ++i;  // skip newline

    try {
        i += parse_block_body(vector_view<Token>(tokens, i), ib);
    } catch (Invalid_syntax& e) {
        throw Traced_syntax_error().append(e).append(
            Invalid_syntax(ib.name, ("in block " + ib.name.str())));
    }

    if (not ib.body.size()) {
        throw Invalid_syntax(ib.name,
                             ("block with empty body: " + ib.name.str()));
    }

    return i;
}

auto viua::assembler::frontend::parser::parse(std::vector<Token> const& tokens)
    -> Parsed_source
{
    Parsed_source parsed;

    for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{0};
         i < tokens.size();
         ++i) {
        if (tokens.at(i) == "\n") {
            continue;
        }
        if (tokens.at(i) == ".function:") {
            Instructions_block ib;
            i += parse_function(vector_view<Token>(tokens, i), ib);
            parsed.functions.push_back(std::move(ib));
        } else if (tokens.at(i) == ".block:") {
            Instructions_block ib;
            i += parse_block(vector_view<Token>(tokens, i), ib);
            parsed.blocks.push_back(std::move(ib));
        } else if (tokens.at(i) == ".closure:") {
            Instructions_block ib;
            i += parse_closure(vector_view<Token>(tokens, i), ib);
            parsed.functions.push_back(std::move(ib));
        } else if (tokens.at(i) == ".signature:") {
            ++i;
            if (tokens.at(i) == "\n") {
                throw Invalid_syntax(tokens.at(i - 1), "missing function name");
            }
            if (not ::assembler::utils::is_valid_function_name(tokens.at(i))) {
                throw Invalid_syntax(tokens.at(i), "not a valid function name");
            }
            parsed.function_signatures.push_back(tokens.at(i++));
        } else if (tokens.at(i) == ".bsignature:") {
            ++i;
            if (tokens.at(i) == "\n") {
                throw Invalid_syntax(tokens.at(i - 1), "missing block name");
            }
            parsed.block_signatures.push_back(tokens.at(i++));
        } else if (tokens.at(i) == ".end") {
            throw Invalid_syntax(tokens.at(i), "stray .end marker");
        } else if (tokens.at(i) == ".info:") {
            // FIXME add meta information to parsed source
            // for now just skip key and value
            i += 2;
        } else if (tokens.at(i) == ".import:") {
            auto const saved_i = i;
            ++i;

            auto attributes = std::map<std::string, std::string>{};
            i += parse_attributes(vector_view<Token>(tokens, i), attributes);

            if (tokens.at(i) == "\n") {
                throw Invalid_syntax(tokens.at(saved_i),
                                     "missing module name in import directive");
            }

            auto name = tokens.at(i);
            ++i;

            if (not parsed.imports.emplace(name.str(), attributes).second) {
                throw Invalid_syntax(tokens.at(i), "double import")
                    .note("module " + name.str() + " was already imported");
            }
        } else if (tokens.at(i).str().at(0) == '.') {
            throw Invalid_syntax(tokens.at(i), "illegal directive")
                .note("expected a function or block definition (or signature)");
        } else {
            throw Invalid_syntax(
                tokens.at(i),
                "expected a function or a block definition (or "
                "signature), or a newline");
        }
    }

    return parsed;
}
