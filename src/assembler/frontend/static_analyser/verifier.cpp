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

#include <algorithm>
#include <viua/assembler/frontend/static_analyser.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/support/string.h>
using namespace viua::assembler::frontend::parser;


using viua::cg::lex::Invalid_syntax;
using viua::cg::lex::Token;
using viua::cg::lex::Traced_syntax_error;


static auto invalid_syntax(std::vector<Token> const& tokens,
                           std::string const message) -> Invalid_syntax
{
    auto invalid_syntax_error = Invalid_syntax(tokens.at(0), message);
    for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{1};
         i < tokens.size();
         ++i) {
        invalid_syntax_error.add(tokens.at(i));
    }
    return invalid_syntax_error;
}


using Verifier = auto (*)(Parsed_source const&, Instructions_block const&)
                     -> void;
static auto verify_wrapper(Parsed_source const& source, Verifier verifier)
    -> void
{
    for (auto const& fn : source.functions) {
        if (fn.attributes.count("no_sa")) {
            continue;
        }
        try {
            verifier(source, fn);
        } catch (Invalid_syntax& e) {
            throw viua::cg::lex::Traced_syntax_error().append(e).append(
                Invalid_syntax(fn.name, ("in function " + fn.name.str())));
        } catch (Traced_syntax_error& e) {
            throw e.append(
                Invalid_syntax(fn.name, ("in function " + fn.name.str())));
        }
    }
    for (auto const& bl : source.blocks) {
        if (bl.attributes.count("no_sa")) {
            continue;
        }
        try {
            verifier(source, bl);
        } catch (Invalid_syntax& e) {
            throw viua::cg::lex::Traced_syntax_error().append(e).append(
                Invalid_syntax(bl.name, ("in block " + bl.name.str())));
        } catch (Traced_syntax_error& e) {
            throw e.append(
                Invalid_syntax(bl.name, ("in block " + bl.name.str())));
        }
    }
}


static auto is_defined_block_name(Parsed_source const& source,
                                  std::string const name) -> bool
{
    auto is_undefined =
        (source.blocks.end()
         == find_if(source.blocks.begin(),
                    source.blocks.end(),
                    [name](Instructions_block const& ib) -> bool {
                        return (ib.name == name);
                    }));
    if (is_undefined) {
        is_undefined = (source.block_signatures.end()
                        == find_if(source.block_signatures.begin(),
                                   source.block_signatures.end(),
                                   [name](Token const& block_name) -> bool {
                                       return (block_name.str() == name);
                                   }));
    }
    return (not is_undefined);
}
auto viua::assembler::frontend::static_analyser::verify_block_tries(
    Parsed_source const& src) -> void
{
    verify_wrapper(
        src,
        [](Parsed_source const& source, Instructions_block const& ib) -> void {
            for (auto const& line : ib.body) {
                auto instruction = dynamic_cast<
                    viua::assembler::frontend::parser::Instruction*>(
                    line.get());
                if (not instruction) {
                    continue;
                }
                if (instruction->opcode != ENTER) {
                    continue;
                }
                auto block_name = instruction->tokens.at(1);

                if (not is_defined_block_name(source, block_name)) {
                    throw Invalid_syntax(
                        block_name,
                        ("cannot enter undefined block: " + block_name.str()));
                }
            }
        });
}
auto viua::assembler::frontend::static_analyser::verify_block_catches(
    Parsed_source const& src) -> void
{
    verify_wrapper(
        src,
        [](Parsed_source const& source, Instructions_block const& ib) -> void {
            for (auto const& line : ib.body) {
                auto instruction = dynamic_cast<
                    viua::assembler::frontend::parser::Instruction*>(
                    line.get());
                if (not instruction) {
                    continue;
                }
                if (instruction->opcode != CATCH) {
                    continue;
                }
                auto block_name = instruction->tokens.at(2);

                if (not is_defined_block_name(source, block_name)) {
                    throw Invalid_syntax(block_name,
                                         ("cannot catch using undefined block: "
                                          + block_name.str()))
                        .add(instruction->tokens.at(0));
                }
            }
        });
}
auto viua::assembler::frontend::static_analyser::verify_block_endings(
    Parsed_source const& src) -> void
{
    verify_wrapper(
        src, [](Parsed_source const&, Instructions_block const& ib) -> void {
            auto last_instruction =
                dynamic_cast<viua::assembler::frontend::parser::Instruction*>(
                    ib.body.back().get());
            if (not last_instruction) {
                throw invalid_syntax(ib.body.back()->tokens,
                                     "invalid end of block: expected mnemonic");
            }
            auto opcode = last_instruction->opcode;
            if (not(opcode == LEAVE or opcode == RETURN
                    or opcode == TAILCALL)) {
                // FIXME throw more specific error (i.e. with different message
                // for blocks and functions)
                throw viua::cg::lex::Invalid_syntax(
                    last_instruction->tokens.at(0), "invalid last mnemonic")
                    .note("expected one of: leave, return, or tailcall");
            }
        });
}
auto viua::assembler::frontend::static_analyser::verify_frame_balance(
    Parsed_source const& src) -> void
{
    verify_wrapper(
        src, [](Parsed_source const&, Instructions_block const& ib) -> void {
            int balance = 0;
            Token previous_frame_spawned;

            for (auto const& line : ib.body) {
                auto instruction = dynamic_cast<
                    viua::assembler::frontend::parser::Instruction*>(
                    line.get());
                if (not instruction) {
                    continue;
                }

                auto opcode = instruction->opcode;
                if (not(opcode == CALL or opcode == TAILCALL or opcode == DEFER
                        or opcode == PROCESS or opcode == WATCHDOG
                        or opcode == FRAME or opcode == RETURN
                        or opcode == LEAVE or opcode == THROW)) {
                    continue;
                }

                if (opcode == CALL or opcode == TAILCALL or opcode == DEFER
                    or opcode == PROCESS or opcode == WATCHDOG) {
                    --balance;
                } else if (opcode == FRAME) {
                    ++balance;
                }

                if (balance < 0) {
                    throw Invalid_syntax(instruction->tokens.at(0),
                                         ("call with '"
                                          + instruction->tokens.at(0).str()
                                          + "' without a frame"));
                }

                if (balance > 1) {
                    throw viua::cg::lex::Traced_syntax_error()
                        .append(Invalid_syntax(instruction->tokens.at(0),
                                               "excess frame spawned"))
                        .append(Invalid_syntax(previous_frame_spawned, "")
                                    .note("unused frame:"));
                }

                if ((opcode == RETURN or opcode == LEAVE or opcode == THROW)
                    and balance > 0) {
                    throw viua::cg::lex::Traced_syntax_error()
                        .append(Invalid_syntax(instruction->tokens.at(0),
                                               "lost frame at:"))
                        .append(Invalid_syntax(previous_frame_spawned, "")
                                    .note("spawned here:"));
                }

                if (opcode == FRAME) {
                    previous_frame_spawned = instruction->tokens.at(0);
                }
            }
        });
}
auto viua::assembler::frontend::static_analyser::verify_function_call_arities(
    Parsed_source const& src) -> void
{
    verify_wrapper(
        src, [](Parsed_source const&, Instructions_block const& ib) -> void {
            int frame_parameters_count = 0;
            Token frame_spawned_here;
            for (auto const& line : ib.body) {
                auto instruction = dynamic_cast<
                    viua::assembler::frontend::parser::Instruction*>(
                    line.get());
                if (not instruction) {
                    continue;
                }

                auto opcode = instruction->opcode;
                auto const relevant_ops = std::set<OPCODE>{
                      CALL
                    , TAILCALL
                    , PROCESS
                    , DEFER
                    , FRAME
                };
                if (not relevant_ops.count(opcode)) {
                    continue;
                }

                using viua::assembler::frontend::parser::Register_index;
                if (opcode == FRAME) {
                    if (dynamic_cast<Register_index*>(
                            instruction->operands.at(0).get())) {
                        frame_parameters_count =
                            static_cast<decltype(frame_parameters_count)>(
                                dynamic_cast<Register_index*>(
                                    instruction->operands.at(0).get())
                                    ->index);
                    } else {
                        frame_parameters_count = -1;
                    }
                    frame_spawned_here = instruction->tokens.at(0);
                    continue;
                }

                viua::assembler::frontend::parser::Operand* operand = nullptr;
                Token operand_token;
                if (opcode == CALL or opcode == PROCESS) {
                    operand = instruction->operands.at(1).get();
                } else if (opcode == DEFER or opcode == TAILCALL) {
                    operand = instruction->operands.at(0).get();
                }

                if (dynamic_cast<Register_index*>(operand)) {
                    // OK, but can't be verified at this time
                    // FIXME verifier should look around and see if it maybe can
                    // check if the function has correct arity; in some cases it
                    // should be possible (if the function was assigned inside
                    // the block being analysed)
                    continue;
                }

                auto function_name = std::string{};
                using viua::assembler::frontend::parser::Atom_literal;
                using viua::assembler::frontend::parser::Function_name_literal;
                if (auto name_from_atom = dynamic_cast<Atom_literal*>(operand);
                    name_from_atom) {
                    function_name = name_from_atom->content;
                } else if (auto name_from_fn =
                               dynamic_cast<Function_name_literal*>(operand);
                           name_from_fn) {
                    function_name = name_from_fn->content;
                } else if (auto label = dynamic_cast<Label*>(operand); label) {
                    throw Invalid_syntax(operand->tokens.at(0),
                                         "not a valid function name")
                        .add(instruction->tokens.at(0));
                } else {
                    throw Invalid_syntax(operand->tokens.at(0),
                                         "invalid operand: expected function "
                                         "name, atom, or register index");
                }

                auto arity =
                    ::assembler::utils::get_function_arity(function_name);

                if (arity >= 0 and arity != frame_parameters_count) {
                    std::ostringstream report;
                    report
                        << "invalid number of parameters in call to function "
                        << function_name << ": expected " << arity << ", got "
                        << frame_parameters_count;
                    throw viua::cg::lex::Traced_syntax_error()
                        .append(Invalid_syntax(instruction->tokens.at(0),
                                               report.str())
                                    .add(operand->tokens.at(0)))
                        .append(Invalid_syntax(frame_spawned_here,
                                               "from frame spawned here"));
                }
            }
        });
}
auto viua::assembler::frontend::static_analyser::verify_frames_have_no_gaps(
    Parsed_source const& src) -> void
{
    verify_wrapper(
        src, [](Parsed_source const&, Instructions_block const& ib) -> void {
            unsigned long frame_parameters_count  = 0;
            auto detected_frame_parameters_count  = false;
            auto slot_index_detection_is_reliable = true;
            viua::assembler::frontend::parser::Instruction* last_frame =
                nullptr;

            auto filled_slots = std::vector<bool>{};
            auto pass_lines   = std::vector<Token>{};

            for (auto const& line : ib.body) {
                auto instruction = dynamic_cast<
                    viua::assembler::frontend::parser::Instruction*>(
                    line.get());

                if (not instruction) {
                    continue;
                }

                auto opcode = instruction->opcode;
                if (not(opcode == CALL or opcode == PROCESS or opcode == DEFER
                        or opcode == FRAME
                        or opcode == MOVE or opcode == COPY)) {
                    continue;
                }

                if (opcode == FRAME) {
                    last_frame = instruction;

                    auto frame_parameters_no_token =
                        last_frame->operands.at(0)->tokens.at(0);
                    if (str::isnum(frame_parameters_no_token.str().substr(1))) {
                        frame_parameters_count =
                            stoul(frame_parameters_no_token.str().substr(1));
                        filled_slots.clear();
                        pass_lines.clear();
                        filled_slots.resize(frame_parameters_count, false);
                        pass_lines.resize(frame_parameters_count);
                        detected_frame_parameters_count = true;
                    } else {
                        detected_frame_parameters_count = false;
                    }
                    slot_index_detection_is_reliable = true;
                    continue;
                }

                auto const& first_operand = *instruction->operands.at(0);
                if ((opcode == MOVE or opcode == COPY)
                    and (first_operand.tokens.size() >= 3 and first_operand.tokens.at(2) == "arguments")) {
                    unsigned long slot_index = 0;
                    bool detected_slot_index = false;

                    auto parameter_index_token =
                        instruction->operands.at(0)->tokens.at(0);
                    if (parameter_index_token.str().at(0) == '@') {
                        slot_index_detection_is_reliable = false;
                    }
                    if (parameter_index_token.str().at(0) == '%'
                        and str::isnum(parameter_index_token.str().substr(1))) {
                        slot_index =
                            stoul(parameter_index_token.str().substr(1));
                        detected_slot_index = true;
                    }

                    if (detected_slot_index and detected_frame_parameters_count
                        and slot_index >= frame_parameters_count) {
                        std::ostringstream report;
                        report << "pass to parameter slot " << slot_index
                               << " in frame with only "
                               << frame_parameters_count << " slots available";
                        throw viua::cg::lex::Invalid_syntax(
                            instruction->tokens.at(0), report.str());
                    }
                    if (detected_slot_index
                        and detected_frame_parameters_count) {
                        if (filled_slots[slot_index]) {
                            throw Traced_syntax_error()
                                .append(Invalid_syntax(
                                            instruction->tokens.at(0),
                                            "double pass to parameter slot "
                                                + std::to_string(slot_index))
                                            .add(instruction->operands.at(0)
                                                     ->tokens.at(0)))
                                .append(
                                    Invalid_syntax(pass_lines[slot_index], "")
                                        .note("first pass at"))
                                .append(
                                    Invalid_syntax(last_frame->tokens.at(0), "")
                                        .note("in frame spawned at"));
                        }
                        filled_slots[slot_index] = true;
                        pass_lines[slot_index]   = instruction->tokens.at(0);
                    }

                    continue;
                }
                if (opcode == MOVE or opcode == COPY) {
                    if (instruction->operands.at(0)->tokens.at(0) == "void") {
                        continue;
                    }
                    if (instruction->operands.at(0)->tokens.at(1)
                        != "arguments") {
                        continue;
                    }

                    unsigned long slot_index = 0;
                    bool detected_slot_index = false;

                    auto parameter_index_token =
                        instruction->operands.at(0)->tokens.at(0);
                    if (parameter_index_token.str().at(0) == '@') {
                        slot_index_detection_is_reliable = false;
                    }
                    if (parameter_index_token.str().at(0) == '%'
                        and str::isnum(parameter_index_token.str().substr(1))) {
                        slot_index =
                            stoul(parameter_index_token.str().substr(1));
                        detected_slot_index = true;
                    }

                    if (detected_slot_index and detected_frame_parameters_count
                        and slot_index >= frame_parameters_count) {
                        std::ostringstream report;
                        report << "pass to parameter slot " << slot_index
                               << " in frame with only "
                               << frame_parameters_count << " slots available";
                        throw viua::cg::lex::Invalid_syntax(
                            instruction->tokens.at(0), report.str());
                    }
                    if (detected_slot_index
                        and detected_frame_parameters_count) {
                        if (filled_slots[slot_index]) {
                            throw Traced_syntax_error()
                                .append(Invalid_syntax(
                                            instruction->tokens.at(0),
                                            "double pass to parameter slot "
                                                + std::to_string(slot_index))
                                            .add(instruction->operands.at(0)
                                                     ->tokens.at(0)))
                                .append(
                                    Invalid_syntax(pass_lines[slot_index], "")
                                        .note("first pass at"))
                                .append(
                                    Invalid_syntax(last_frame->tokens.at(0), "")
                                        .note("in frame spawned at"));
                        }
                        filled_slots[slot_index] = true;
                        pass_lines[slot_index]   = instruction->tokens.at(0);
                    }

                    continue;
                }

                if (slot_index_detection_is_reliable) {
                    for (auto j = decltype(frame_parameters_count){0};
                         j < frame_parameters_count;
                         ++j) {
                        if (not filled_slots[j]) {
                            throw Traced_syntax_error()
                                .append(Invalid_syntax(last_frame->tokens.at(0),
                                                       "gap in frame"))
                                .append(
                                    Invalid_syntax(instruction->tokens.at(0),
                                                   ("slot " + std::to_string(j)
                                                    + " left empty at")));
                        }
                    }
                }
            }
        });
}

using InstructionIndex = decltype(
    viua::assembler::frontend::parser::Instructions_block::body)::size_type;
static auto validate_jump(
    const Token token,
    std::string const& extracted_jump,
    const InstructionIndex instruction_counter,
    const InstructionIndex current_instruction_counter,
    const std::map<std::string, InstructionIndex>& jump_targets)
    -> InstructionIndex
{
    auto target = InstructionIndex{0};
    if (str::isnum(extracted_jump, false)) {
        target = stoul(extracted_jump);
    } else if (str::startswith(extracted_jump, "+")
               and str::isnum(extracted_jump.substr(1), false)) {
        auto jump_offset = stoul(extracted_jump.substr(1));
        if (jump_offset == 0) {
            throw viua::cg::lex::Invalid_syntax(token, "zero-distance jump");
        }
        target = (current_instruction_counter + jump_offset);
    } else if (str::startswith(extracted_jump, "-")
               and str::isnum(extracted_jump.substr(1), false)) {
        auto jump_offset = stoul(extracted_jump.substr(1));
        if (jump_offset == 0) {
            throw viua::cg::lex::Invalid_syntax(token, "zero-distance jump");
        }
        if (jump_offset > current_instruction_counter) {
            throw Invalid_syntax(token, "backward out-of-range jump");
        }
        target = (current_instruction_counter - jump_offset);
    } else if (str::ishex(extracted_jump)) {
        // absolute jumps cannot be verified without knowing how many bytes the
        // bytecode spans this is a FIXME: add check for absolute jumps
        return stoul(extracted_jump, nullptr, 16);
    } else if (str::isid(extracted_jump)) {
        if (jump_targets.count(extracted_jump) == 0) {
            throw viua::cg::lex::Invalid_syntax(
                token, ("jump to unrecognised marker: " + extracted_jump));
        }
        target = jump_targets.at(extracted_jump);
        if (target > instruction_counter) {
            throw viua::cg::lex::Invalid_syntax(token,
                                                "marker out-of-range jump");
        }
    } else {
        throw viua::cg::lex::Invalid_syntax(
            token, "invalid operand for jump instruction")
            .note("expected a label or an offset");
    }

    if (target == current_instruction_counter) {
        throw viua::cg::lex::Invalid_syntax(token, "zero-distance jump");
    }
    if (target > instruction_counter) {
        throw viua::cg::lex::Invalid_syntax(token, "forward out-of-range jump");
    }

    return target;
}
static auto validate_jump_pair(
    Token const& branch_token,
    Token const& when_true,
    Token const& when_false,
    const InstructionIndex instruction_counter,
    const InstructionIndex current_instruction_counter,
    const std::map<std::string, InstructionIndex>& jump_targets) -> void
{
    if (when_true.str() == when_false.str()) {
        throw viua::cg::lex::Invalid_syntax(
            branch_token,
            "useless branch: both targets point to the same instruction");
    }

    auto true_target  = validate_jump(when_true,
                                     when_true.str(),
                                     instruction_counter,
                                     current_instruction_counter,
                                     jump_targets);
    auto false_target = validate_jump(when_false,
                                      when_false.str(),
                                      instruction_counter,
                                      current_instruction_counter,
                                      jump_targets);

    if (true_target == false_target) {
        throw viua::cg::lex::Invalid_syntax(
            branch_token,
            "useless branch: both targets point to the same instruction");
    }
}
auto viua::assembler::frontend::static_analyser::verify_jumps_are_in_range(
    Parsed_source const& src) -> void
{
    verify_wrapper(
        src, [](Parsed_source const&, Instructions_block const& ib) -> void {
            // XXX HACK start from maximum value, and wrap to zero when
            // incremented for first instruction; this is a hack
            auto instruction_counter = static_cast<InstructionIndex>(-1);
            auto current_instruction_counter =
                static_cast<InstructionIndex>(-1);

            std::map<std::string, decltype(instruction_counter)> jump_targets;

            for (auto const& line : ib.body) {
                auto mnemonic = line->tokens.at(0);

                using viua::assembler::frontend::parser::Directive;
                if (auto mark = dynamic_cast<Directive*>(line.get());
                    mark and mark->directive == ".mark:") {
                    jump_targets[mark->operands.at(0)] =
                        instruction_counter + 1;  // marker points at the *next*
                                                  // instruction
                }

                if (mnemonic.str().at(0) != '.') {
                    ++instruction_counter;
                }
            }

            for (auto const& line : ib.body) {
                auto mnemonic = line->tokens.at(0);
                if (mnemonic.str().at(0) != '.') {
                    ++current_instruction_counter;
                }

                using viua::assembler::frontend::parser::Instruction;
                if (mnemonic == "jump") {
                    validate_jump(line->tokens.at(1),
                                  line->tokens.at(1),
                                  instruction_counter,
                                  current_instruction_counter,
                                  jump_targets);
                } else if (auto op = dynamic_cast<Instruction*>(line.get());
                           op and op->opcode == IF) {
                    Token when_true  = op->operands.at(1)->tokens.at(0);
                    Token when_false = op->operands.at(2)->tokens.at(0);

                    validate_jump_pair(line->tokens.at(0),
                                       when_true,
                                       when_false,
                                       instruction_counter,
                                       current_instruction_counter,
                                       jump_targets);
                }
            }
        });
}


auto viua::assembler::frontend::static_analyser::verify(
    Parsed_source const& source) -> void
{
    verify_block_tries(source);
    verify_block_catches(source);
    verify_block_endings(source);
    verify_frame_balance(source);
    verify_function_call_arities(source);
    verify_frames_have_no_gaps(source);
    verify_jumps_are_in_range(source);
}


static std::set<viua::assembler::frontend::static_analyser::Reportable_error>
    allowed_errors;
auto viua::assembler::frontend::static_analyser::allowed_error(
    Reportable_error const error) -> bool
{
    return (allowed_errors.count(error) != 0);
}

auto viua::assembler::frontend::static_analyser::allow_error(
    Reportable_error const error,
    bool const allow) -> void
{
    if (allow) {
        allowed_errors.insert(error);
    } else {
        allowed_errors.erase(allowed_errors.find(error));
    }
}

auto viua::assembler::frontend::static_analyser::to_reportable_error(
    std::string_view const s) -> std::optional<Reportable_error>
{
    if (s == "-Wunused-register") {
        return Reportable_error::Unused_register;
    } else if (s == "-Wunused-value") {
        return Reportable_error::Unused_value;
    }
    return {};
}
