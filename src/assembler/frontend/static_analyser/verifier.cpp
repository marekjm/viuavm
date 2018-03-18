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

#include <algorithm>
#include <viua/assembler/frontend/static_analyser.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/support/string.h>
using namespace std;
using namespace viua::assembler::frontend::parser;


using viua::cg::lex::InvalidSyntax;
using viua::cg::lex::Token;
using viua::cg::lex::TracedSyntaxError;


static auto invalid_syntax(const vector<Token>& tokens, const string message) -> InvalidSyntax {
    auto invalid_syntax_error = InvalidSyntax(tokens.at(0), message);
    for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{1}; i < tokens.size(); ++i) {
        invalid_syntax_error.add(tokens.at(i));
    }
    return invalid_syntax_error;
}


using Verifier = auto (*)(const ParsedSource&, const InstructionsBlock&) -> void;
static auto verify_wrapper(const ParsedSource& source, Verifier verifier) -> void {
    for (const auto& fn : source.functions) {
        if (fn.attributes.count("no_sa")) {
            continue;
        }
        try {
            verifier(source, fn);
        } catch (InvalidSyntax& e) {
            throw viua::cg::lex::TracedSyntaxError().append(e).append(
                InvalidSyntax(fn.name, ("in function " + fn.name.str())));
        } catch (TracedSyntaxError& e) {
            throw e.append(InvalidSyntax(fn.name, ("in function " + fn.name.str())));
        }
    }
    for (const auto& bl : source.blocks) {
        if (bl.attributes.count("no_sa")) {
            continue;
        }
        try {
            verifier(source, bl);
        } catch (InvalidSyntax& e) {
            throw viua::cg::lex::TracedSyntaxError().append(e).append(
                InvalidSyntax(bl.name, ("in block " + bl.name.str())));
        } catch (TracedSyntaxError& e) {
            throw e.append(InvalidSyntax(bl.name, ("in block " + bl.name.str())));
        }
    }
}


auto viua::assembler::frontend::static_analyser::verify_ress_instructions(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource& source, const InstructionsBlock& ib) -> void {
        for (const auto& line : ib.body) {
            const auto instruction =
                dynamic_cast<viua::assembler::frontend::parser::Instruction*>(line.get());
            if (not instruction) {
                continue;
            }
            if (instruction->opcode != RESS) {
                continue;
            }
            const auto label =
                dynamic_cast<viua::assembler::frontend::parser::Label*>(instruction->operands.at(0).get());
            if (not label) {
                throw invalid_syntax(instruction->operands.at(0)->tokens,
                                     "illegal operand for 'ress' instruction")
                    .note("expected register set name");
            }

            auto name = label->content;
            if (not(name == "global" or name == "static" or name == "local")) {
                auto error = invalid_syntax(instruction->operands.at(0)->tokens, "not a register set name");
                if (auto suggestion = str::levenshtein_best(name,
                                                            {
                                                                "global",
                                                                "static",
                                                                "local",
                                                            },
                                                            4);
                    suggestion.first) {
                    error.aside(label->tokens.at(0), "did you mean '" + suggestion.second + "'?");
                }
                throw error;
            }
            if (name == "global" and source.as_library) {
                throw invalid_syntax(instruction->operands.at(0)->tokens,
                                     "global register set used by a library function")
                    .note("library functions may only use 'local' and 'static' register sets");
            }
        }
    });
}
static auto is_defined_block_name(const ParsedSource& source, const string name) -> bool {
    auto is_undefined = (source.blocks.end() ==
                         find_if(source.blocks.begin(), source.blocks.end(),
                                 [name](const InstructionsBlock& ib) -> bool { return (ib.name == name); }));
    if (is_undefined) {
        is_undefined =
            (source.block_signatures.end() ==
             find_if(source.block_signatures.begin(), source.block_signatures.end(),
                     [name](const Token& block_name) -> bool { return (block_name.str() == name); }));
    }
    return (not is_undefined);
}
auto viua::assembler::frontend::static_analyser::verify_block_tries(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource& source, const InstructionsBlock& ib) -> void {
        for (const auto& line : ib.body) {
            auto instruction = dynamic_cast<viua::assembler::frontend::parser::Instruction*>(line.get());
            if (not instruction) {
                continue;
            }
            if (instruction->opcode != ENTER) {
                continue;
            }
            auto block_name = instruction->tokens.at(1);

            if (not is_defined_block_name(source, block_name)) {
                throw InvalidSyntax(block_name, ("cannot enter undefined block: " + block_name.str()));
            }
        }
    });
}
auto viua::assembler::frontend::static_analyser::verify_block_catches(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource& source, const InstructionsBlock& ib) -> void {
        for (const auto& line : ib.body) {
            auto instruction = dynamic_cast<viua::assembler::frontend::parser::Instruction*>(line.get());
            if (not instruction) {
                continue;
            }
            if (instruction->opcode != CATCH) {
                continue;
            }
            auto block_name = instruction->tokens.at(2);

            if (not is_defined_block_name(source, block_name)) {
                throw InvalidSyntax(block_name, ("cannot catch using undefined block: " + block_name.str()))
                    .add(instruction->tokens.at(0));
            }
        }
    });
}
auto viua::assembler::frontend::static_analyser::verify_block_endings(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource&, const InstructionsBlock& ib) -> void {
        auto last_instruction =
            dynamic_cast<viua::assembler::frontend::parser::Instruction*>(ib.body.back().get());
        if (not last_instruction) {
            throw invalid_syntax(ib.body.back()->tokens, "invalid end of block: expected mnemonic");
        }
        auto opcode = last_instruction->opcode;
        if (not(opcode == LEAVE or opcode == RETURN or opcode == TAILCALL)) {
            // FIXME throw more specific error (i.e. with different message for blocks and functions)
            throw viua::cg::lex::InvalidSyntax(last_instruction->tokens.at(0), "invalid last mnemonic")
                .note("expected one of: leave, return, or tailcall");
        }
    });
}
auto viua::assembler::frontend::static_analyser::verify_frame_balance(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource&, const InstructionsBlock& ib) -> void {
        int balance = 0;
        Token previous_frame_spawned;

        for (const auto& line : ib.body) {
            auto instruction = dynamic_cast<viua::assembler::frontend::parser::Instruction*>(line.get());
            if (not instruction) {
                continue;
            }

            auto opcode = instruction->opcode;
            if (not(opcode == CALL or opcode == TAILCALL or opcode == DEFER or opcode == PROCESS or
                    opcode == FRAME or opcode == MSG or opcode == RETURN or opcode == LEAVE or
                    opcode == THROW)) {
                continue;
            }

            if (opcode == CALL or opcode == TAILCALL or opcode == DEFER or opcode == PROCESS or opcode == MSG) {
                --balance;
            } else if (opcode == FRAME) {
                ++balance;
            }

            if (balance < 0) {
                throw InvalidSyntax(instruction->tokens.at(0),
                                    ("call with '" + instruction->tokens.at(0).str() + "' without a frame"));
            }

            if (balance > 1) {
                throw viua::cg::lex::TracedSyntaxError()
                    .append(InvalidSyntax(instruction->tokens.at(0), "excess frame spawned"))
                    .append(InvalidSyntax(previous_frame_spawned, "").note("unused frame:"));
            }

            if ((opcode == RETURN or opcode == LEAVE or opcode == THROW) and balance > 0) {
                throw viua::cg::lex::TracedSyntaxError()
                    .append(InvalidSyntax(instruction->tokens.at(0), "lost frame at:"))
                    .append(InvalidSyntax(previous_frame_spawned, "").note("spawned here:"));
            }

            if (opcode == FRAME) {
                previous_frame_spawned = instruction->tokens.at(0);
            }
        }
    });
}
auto viua::assembler::frontend::static_analyser::verify_function_call_arities(const ParsedSource& src)
    -> void {
    verify_wrapper(src, [](const ParsedSource&, const InstructionsBlock& ib) -> void {
        int frame_parameters_count = 0;
        Token frame_spawned_here;
        for (const auto& line : ib.body) {
            auto instruction = dynamic_cast<viua::assembler::frontend::parser::Instruction*>(line.get());
            if (not instruction) {
                continue;
            }

            auto opcode = instruction->opcode;
            if (not(opcode == CALL or opcode == PROCESS or opcode == DEFER or opcode == MSG or
                    opcode == FRAME)) {
                continue;
            }

            using viua::assembler::frontend::parser::RegisterIndex;
            if (opcode == FRAME) {
                if (dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get())) {
                    frame_parameters_count = static_cast<decltype(frame_parameters_count)>(
                        dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get())->index);
                } else {
                    frame_parameters_count = -1;
                }
                frame_spawned_here = instruction->tokens.at(0);
                continue;
            }

            viua::assembler::frontend::parser::Operand* operand = nullptr;
            Token operand_token;
            if (opcode == CALL or opcode == PROCESS or opcode == MSG) {
                operand = instruction->operands.at(1).get();
            } else if (opcode == DEFER) {
                operand = instruction->operands.at(0).get();
            }

            if (dynamic_cast<RegisterIndex*>(operand)) {
                // OK, but can't be verified at this time
                // FIXME verifier should look around and see if it maybe can check if the function has
                // correct arity; in some cases it should be possible (if the function was assigned inside
                // the block being analysed)
                continue;
            }

            string function_name;
            using viua::assembler::frontend::parser::AtomLiteral;
            using viua::assembler::frontend::parser::FunctionNameLiteral;
            if (auto name_from_atom = dynamic_cast<AtomLiteral*>(operand); name_from_atom) {
                function_name = name_from_atom->content;
            } else if (auto name_from_fn = dynamic_cast<FunctionNameLiteral*>(operand); name_from_fn) {
                function_name = name_from_fn->content;
            } else if (auto label = dynamic_cast<Label*>(operand); label) {
                throw InvalidSyntax(operand->tokens.at(0), "not a valid function name")
                    .add(instruction->tokens.at(0));
            } else {
                throw InvalidSyntax(operand->tokens.at(0),
                                    "invalid operand: expected function name, atom, or register index");
            }

            if (opcode == MSG and frame_parameters_count == 0) {
                throw InvalidSyntax(instruction->tokens.at(0),
                                    "invalid number of parameters in dynamic dispatch")
                    .note("expected at least 1 parameter, got 0")
                    .add(operand->tokens.at(0));
            }

            auto arity = ::assembler::utils::get_function_arity(function_name);

            if (arity >= 0 and arity != frame_parameters_count) {
                ostringstream report;
                report << "invalid number of parameters in call to function " << function_name
                       << ": expected " << arity << ", got " << frame_parameters_count;
                throw viua::cg::lex::TracedSyntaxError()
                    .append(InvalidSyntax(instruction->tokens.at(0), report.str()).add(operand->tokens.at(0)))
                    .append(InvalidSyntax(frame_spawned_here, "from frame spawned here"));
            }
        }
    });
}
auto viua::assembler::frontend::static_analyser::verify_frames_have_no_gaps(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource&, const InstructionsBlock& ib) -> void {
        unsigned long frame_parameters_count = 0;
        auto detected_frame_parameters_count = false;
        auto slot_index_detection_is_reliable = true;
        viua::assembler::frontend::parser::Instruction* last_frame = nullptr;

        vector<bool> filled_slots;
        vector<Token> pass_lines;

        for (const auto& line : ib.body) {
            auto instruction = dynamic_cast<viua::assembler::frontend::parser::Instruction*>(line.get());

            if (not instruction) {
                continue;
            }

            auto opcode = instruction->opcode;
            if (not(opcode == CALL or opcode == PROCESS or opcode == MSG or opcode == DEFER or
                    opcode == FRAME or opcode == PARAM or opcode == PAMV)) {
                continue;
            }

            if (opcode == FRAME) {
                last_frame = instruction;

                auto frame_parameters_no_token = last_frame->operands.at(0)->tokens.at(0);
                if (str::isnum(frame_parameters_no_token.str().substr(1))) {
                    frame_parameters_count = stoul(frame_parameters_no_token.str().substr(1));
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

            if (opcode == PARAM or opcode == PAMV) {
                unsigned long slot_index = 0;
                bool detected_slot_index = false;

                auto parameter_index_token = instruction->operands.at(0)->tokens.at(0);
                if (parameter_index_token.str().at(0) == '@') {
                    slot_index_detection_is_reliable = false;
                }
                if (parameter_index_token.str().at(0) == '%' and
                    str::isnum(parameter_index_token.str().substr(1))) {
                    slot_index = stoul(parameter_index_token.str().substr(1));
                    detected_slot_index = true;
                }

                if (detected_slot_index and detected_frame_parameters_count and
                    slot_index >= frame_parameters_count) {
                    ostringstream report;
                    report << "pass to parameter slot " << slot_index << " in frame with only "
                           << frame_parameters_count << " slots available";
                    throw viua::cg::lex::InvalidSyntax(instruction->tokens.at(0), report.str());
                }
                if (detected_slot_index and detected_frame_parameters_count) {
                    if (filled_slots[slot_index]) {
                        throw TracedSyntaxError()
                            .append(InvalidSyntax(instruction->tokens.at(0),
                                                  "double pass to parameter slot " + to_string(slot_index))

                                        .add(instruction->operands.at(0)->tokens.at(0)))
                            .append(InvalidSyntax(pass_lines[slot_index], "").note("first pass at"))
                            .append(InvalidSyntax(last_frame->tokens.at(0), "").note("in frame spawned at"));
                    }
                    filled_slots[slot_index] = true;
                    pass_lines[slot_index] = instruction->tokens.at(0);
                }

                continue;
            }

            if (slot_index_detection_is_reliable) {
                for (auto j = decltype(frame_parameters_count){0}; j < frame_parameters_count; ++j) {
                    if (not filled_slots[j]) {
                        throw TracedSyntaxError()
                            .append(InvalidSyntax(last_frame->tokens.at(0), "gap in frame"))
                            .append(InvalidSyntax(instruction->tokens.at(0),
                                                  ("slot " + to_string(j) + " left empty at")));
                    }
                }
            }
        }
    });
}

using InstructionIndex = decltype(viua::assembler::frontend::parser::InstructionsBlock::body)::size_type;
static auto validate_jump(const Token token, const string& extracted_jump,
                          const InstructionIndex instruction_counter,
                          const InstructionIndex current_instruction_counter,
                          const map<string, InstructionIndex>& jump_targets) -> InstructionIndex {
    auto target = InstructionIndex{0};
    if (str::isnum(extracted_jump, false)) {
        target = stoul(extracted_jump);
    } else if (str::startswith(extracted_jump, "+") and str::isnum(extracted_jump.substr(1), false)) {
        auto jump_offset = stoul(extracted_jump.substr(1));
        if (jump_offset == 0) {
            throw viua::cg::lex::InvalidSyntax(token, "zero-distance jump");
        }
        target = (current_instruction_counter + jump_offset);
    } else if (str::startswith(extracted_jump, "-") and str::isnum(extracted_jump.substr(1), false)) {
        auto jump_offset = stoul(extracted_jump.substr(1));
        if (jump_offset == 0) {
            throw viua::cg::lex::InvalidSyntax(token, "zero-distance jump");
        }
        if (jump_offset > current_instruction_counter) {
            throw InvalidSyntax(token, "backward out-of-range jump");
        }
        target = (current_instruction_counter - jump_offset);
    } else if (str::ishex(extracted_jump)) {
        // absolute jumps cannot be verified without knowing how many bytes the bytecode spans
        // this is a FIXME: add check for absolute jumps
        return stoul(extracted_jump, nullptr, 16);
    } else if (str::isid(extracted_jump)) {
        if (jump_targets.count(extracted_jump) == 0) {
            throw viua::cg::lex::InvalidSyntax(token, ("jump to unrecognised marker: " + extracted_jump));
        }
        target = jump_targets.at(extracted_jump);
        if (target > instruction_counter) {
            throw viua::cg::lex::InvalidSyntax(token, "marker out-of-range jump");
        }
    } else {
        throw viua::cg::lex::InvalidSyntax(token, "invalid operand for jump instruction")
            .note("expected a label or an offset");
    }

    if (target == current_instruction_counter) {
        throw viua::cg::lex::InvalidSyntax(token, "zero-distance jump");
    }
    if (target > instruction_counter) {
        throw viua::cg::lex::InvalidSyntax(token, "forward out-of-range jump");
    }

    return target;
}
static auto validate_jump_pair(const Token& branch_token, const Token& when_true, const Token& when_false,
                               const InstructionIndex instruction_counter,
                               const InstructionIndex current_instruction_counter,
                               const map<string, InstructionIndex>& jump_targets) -> void {
    if (when_true.str() == when_false.str()) {
        throw viua::cg::lex::InvalidSyntax(branch_token,
                                           "useless branch: both targets point to the same instruction");
    }

    auto true_target = validate_jump(when_true, when_true.str(), instruction_counter,
                                     current_instruction_counter, jump_targets);
    auto false_target = validate_jump(when_false, when_false.str(), instruction_counter,
                                      current_instruction_counter, jump_targets);

    if (true_target == false_target) {
        throw viua::cg::lex::InvalidSyntax(branch_token,
                                           "useless branch: both targets point to the same instruction");
    }
}
auto viua::assembler::frontend::static_analyser::verify_jumps_are_in_range(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource&, const InstructionsBlock& ib) -> void {
        // XXX HACK start from maximum value, and wrap to zero when
        // incremented for first instruction; this is a hack
        auto instruction_counter = static_cast<InstructionIndex>(-1);
        auto current_instruction_counter = static_cast<InstructionIndex>(-1);

        map<string, decltype(instruction_counter)> jump_targets;

        for (const auto& line : ib.body) {
            auto mnemonic = line->tokens.at(0);

            using viua::assembler::frontend::parser::Directive;
            if (auto mark = dynamic_cast<Directive*>(line.get()); mark and mark->directive == ".mark:") {
                jump_targets[mark->operands.at(0)] =
                    instruction_counter + 1;  // marker points at the *next* instruction
            }

            if (mnemonic.str().at(0) != '.') {
                ++instruction_counter;
            }
        }

        for (const auto& line : ib.body) {
            auto mnemonic = line->tokens.at(0);
            if (mnemonic.str().at(0) != '.') {
                ++current_instruction_counter;
            }

            using viua::assembler::frontend::parser::Instruction;
            if (mnemonic == "jump") {
                validate_jump(line->tokens.at(1), line->tokens.at(1), instruction_counter,
                              current_instruction_counter, jump_targets);
            } else if (auto op = dynamic_cast<Instruction*>(line.get()); op and op->opcode == IF) {
                Token when_true = op->operands.at(1)->tokens.at(0);
                Token when_false = op->operands.at(2)->tokens.at(0);

                validate_jump_pair(line->tokens.at(0), when_true, when_false, instruction_counter,
                                   current_instruction_counter, jump_targets);
            }
        }
    });
}


auto viua::assembler::frontend::static_analyser::verify(const ParsedSource& source) -> void {
    verify_ress_instructions(source);
    verify_block_tries(source);
    verify_block_catches(source);
    verify_block_endings(source);
    verify_frame_balance(source);
    verify_function_call_arities(source);
    verify_frames_have_no_gaps(source);
    verify_jumps_are_in_range(source);
}
