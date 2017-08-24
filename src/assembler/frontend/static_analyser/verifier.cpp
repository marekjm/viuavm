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
using viua::cg::lex::Token;
using viua::cg::lex::InvalidSyntax;
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
            if (not(label->content == "global" or label->content == "static" or label->content == "local")) {
                throw invalid_syntax(instruction->operands.at(0)->tokens, "not a register set name");
            }
            if (label->content == "global" and source.as_library) {
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
            throw viua::cg::lex::InvalidSyntax(
                last_instruction->tokens.at(0),
                "invalid last mnemonic: expected one of: leave, return, or tailcall");
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

            switch (opcode) {
                case CALL:
                case TAILCALL:
                case DEFER:
                case PROCESS:
                case MSG:
                    --balance;
                    break;
                case FRAME:
                    ++balance;
                    break;
                default:
                    break;
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

            if (opcode == FRAME) {
                if (dynamic_cast<viua::assembler::frontend::parser::RegisterIndex*>(
                        instruction->operands.at(0).get())) {
                    frame_parameters_count = static_cast<decltype(frame_parameters_count)>(
                        dynamic_cast<viua::assembler::frontend::parser::RegisterIndex*>(
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
            if (opcode == CALL or opcode == PROCESS or opcode == MSG) {
                operand = instruction->operands.at(1).get();
            } else if (opcode == DEFER) {
                operand = instruction->operands.at(0).get();
            }

            if (dynamic_cast<viua::assembler::frontend::parser::RegisterIndex*>(operand)) {
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

            auto arity = ::assembler::utils::getFunctionArity(function_name);

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
        bool detected_frame_parameters_count = false;
        bool slot_index_detection_is_reliable = true;
        viua::assembler::frontend::parser::Instruction* last_frame;

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
                          const InstructionIndex function_instruction_counter,
                          vector<pair<Token, InstructionIndex>>& forward_jumps,
                          vector<pair<Token, string>>& deferred_marker_jumps,
                          const map<string, InstructionIndex>& jump_targets) -> void {
    long int target = -1;
    if (str::isnum(extracted_jump, false)) {
        target = stoi(extracted_jump);
    } else if (str::startswith(extracted_jump, "+") and str::isnum(extracted_jump.substr(1))) {
        int jump_offset = stoi(extracted_jump.substr(1));
        if (jump_offset == 0) {
            throw viua::cg::lex::InvalidSyntax(token, "zero-distance jump");
        }
        target = (function_instruction_counter + jump_offset);
    } else if (str::startswith(extracted_jump, "-") and str::isnum(extracted_jump)) {
        int jump_offset = stoi(extracted_jump);
        if (jump_offset == 0) {
            throw viua::cg::lex::InvalidSyntax(token, "zero-distance jump");
        }
        target = (function_instruction_counter + jump_offset);
    } else if (str::startswith(extracted_jump, ".") and str::isnum(extracted_jump.substr(1))) {
        target = stoi(extracted_jump.substr(1));
        if (target < 0) {
            throw viua::cg::lex::InvalidSyntax(token, "absolute jump with negative value");
        }
        if (target == 0 and function_instruction_counter == 0) {
            throw viua::cg::lex::InvalidSyntax(token, "zero-distance jump");
        }
        // absolute jumps cannot be verified without knowing how many bytes the bytecode spans
        // this is a FIXME: add check for absolute jumps
        return;
    } else if (str::ishex(extracted_jump)) {
        // absolute jumps cannot be verified without knowing how many bytes the bytecode spans
        // this is a FIXME: add check for absolute jumps
        return;
    } else {
        if (jump_targets.count(extracted_jump) == 0) {
            deferred_marker_jumps.emplace_back(token, extracted_jump);
            return;
        } else {
            // FIXME: jump targets are saved with an off-by-one error, that surfaces when
            // a .mark: directive immediately follows .function: declaration
            target = jump_targets.at(extracted_jump) + 1;
        }
    }

    if (target < 0) {
        throw viua::cg::lex::InvalidSyntax(token, "backward out-of-range jump");
    }
    if (static_cast<InstructionIndex>(target) == function_instruction_counter) {
        throw viua::cg::lex::InvalidSyntax(token, "zero-distance jump");
    }
    if (static_cast<InstructionIndex>(target) > function_instruction_counter) {
        forward_jumps.emplace_back(token, target);
    }
}
static auto verify_forward_jumps(const InstructionIndex function_instruction_counter,
                                 const vector<pair<Token, InstructionIndex>>& forward_jumps) -> void {
    for (auto jmp : forward_jumps) {
        if (jmp.second > function_instruction_counter) {
            throw viua::cg::lex::InvalidSyntax(jmp.first, "forward out-of-range jump");
        }
    }
}
static auto validate_jump_pair(
    const Token& branch_token, const Token& when_true, const Token& when_false,
    InstructionIndex function_instruction_counter,
    vector<tuple<Token, Token, Token, InstructionIndex>>& deferred_jump_pair_checks,
    const map<string, InstructionIndex>& jump_targets) -> void {
    if (when_true.str() == when_false.str()) {
        throw viua::cg::lex::InvalidSyntax(branch_token,
                                           "useless branch: both targets point to the same instruction");
    }

    InstructionIndex true_target = 0, false_target = 0;
    if (str::isnum(when_true, false)) {
        true_target = stoi(when_true);
    } else if (str::startswith(when_true, "+") and str::isnum(when_true.str().substr(1))) {
        int jump_offset = stoi(when_true.str().substr(1));
        true_target = (function_instruction_counter + jump_offset);
    } else if (str::startswith(when_true, "-") and str::isnum(when_true)) {
        int jump_offset = stoi(when_true);
        true_target = (function_instruction_counter + jump_offset);
    } else if (str::ishex(when_true) or str::startswith(when_true, ".")) {
        // absolute jumps cannot be verified without knowing how many bytes the bytecode spans
        // this is a FIXME: add check for absolute jumps
        return;
    } else {
        if (jump_targets.count(when_true) == 0) {
            deferred_jump_pair_checks.emplace_back(branch_token, when_true, when_false,
                                                   function_instruction_counter);
            return;
        } else {
            // FIXME: jump targets are saved with an off-by-one error, that surfaces when
            // a .mark: directive immediately follows .function: declaration
            true_target = jump_targets.at(when_true) + 1;
        }
    }

    if (str::isnum(when_false, false)) {
        false_target = stoi(when_false);
    } else if (str::startswith(when_false, "+") and str::isnum(when_false.str().substr(1))) {
        int jump_offset = stoi(when_false.str().substr(1));
        false_target = (function_instruction_counter + jump_offset);
    } else if (str::startswith(when_false, "-") and str::isnum(when_false)) {
        int jump_offset = stoi(when_false);
        false_target = (function_instruction_counter + jump_offset);
    } else if (str::ishex(when_false) or str::startswith(when_true, ".")) {
        // absolute jumps cannot be verified without knowing how many bytes the bytecode spans
        // this is a FIXME: add check for absolute jumps
        return;
    } else {
        if (jump_targets.count(when_false) == 0) {
            deferred_jump_pair_checks.emplace_back(branch_token, when_true, when_false,
                                                   function_instruction_counter);
            return;
        } else {
            // FIXME: jump targets are saved with an off-by-one error, that surfaces when
            // a .mark: directive immediately follows .function: declaration
            false_target = jump_targets.at(when_false) + 1;
        }
    }

    if (true_target == false_target) {
        throw viua::cg::lex::InvalidSyntax(branch_token,
                                           "useless branch: both targets point to the same instruction");
    }
}
static auto verify_deferred_jump_pairs(
    const vector<tuple<Token, Token, Token, InstructionIndex>>& deferred_jump_pair_checks,
    const map<string, InstructionIndex>& jump_targets) -> void {
    for (const auto& each : deferred_jump_pair_checks) {
        InstructionIndex function_instruction_counter = 0;
        Token branch_token, when_true, when_false;
        tie(branch_token, when_true, when_false, function_instruction_counter) = each;

        InstructionIndex true_target = 0, false_target = 0;
        if (str::isnum(when_true, false)) {
            true_target = stoi(when_true);
        } else if (str::startswith(when_true, "+") and str::isnum(when_true.str().substr(1))) {
            int jump_offset = stoi(when_true.str().substr(1));
            true_target = (function_instruction_counter + jump_offset);
        } else if (str::startswith(when_true, "-") and str::isnum(when_true)) {
            int jump_offset = stoi(when_true);
            true_target = (function_instruction_counter + jump_offset);
        } else {
            // FIXME: jump targets are saved with an off-by-one error, that surfaces when
            // a .mark: directive immediately follows .function: declaration
            true_target = jump_targets.at(when_true) + 1;
        }

        if (str::isnum(when_false, false)) {
            false_target = stoi(when_false);
        } else if (str::startswith(when_false, "+") and str::isnum(when_false.str().substr(1))) {
            int jump_offset = stoi(when_false.str().substr(1));
            false_target = (function_instruction_counter + jump_offset);
        } else if (str::startswith(when_false, "-") and str::isnum(when_false)) {
            int jump_offset = stoi(when_false);
            false_target = (function_instruction_counter + jump_offset);
        } else {
            // FIXME: jump targets are saved with an off-by-one error, that surfaces when
            // a .mark: directive immediately follows .function: declaration
            false_target = jump_targets.at(when_false) + 1;
        }

        if (true_target == false_target) {
            throw viua::cg::lex::InvalidSyntax(branch_token,
                                               "useless branch: both targets point to the same instruction");
        }
    }
}
static auto verify_marker_jumps(const InstructionIndex function_instruction_counter,
                                const vector<pair<Token, string>>& deferred_marker_jumps,
                                const map<string, InstructionIndex>& jump_targets) -> void {
    for (auto jmp : deferred_marker_jumps) {
        if (jump_targets.count(jmp.second) == 0) {
            throw viua::cg::lex::InvalidSyntax(jmp.first, ("jump to unrecognised marker: " + jmp.second));
        }
        if (jump_targets.at(jmp.second) > function_instruction_counter) {
            throw viua::cg::lex::InvalidSyntax(jmp.first, "marker out-of-range jump");
        }
    }
}
auto viua::assembler::frontend::static_analyser::verify_jumps_are_in_range(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource&, const InstructionsBlock& ib) -> void {
        auto function_instruction_counter = InstructionIndex{0};
        map<string, decltype(function_instruction_counter)> jump_targets;
        vector<pair<Token, InstructionIndex>> forward_jumps;
        vector<pair<Token, string>> deferred_marker_jumps;
        vector<tuple<Token, Token, Token, InstructionIndex>> deferred_jump_pair_checks;

        for (const auto& line : ib.body) {
            auto mnemonic = line->tokens.at(0);
            if (mnemonic.str().at(0) != '.') {
                ++function_instruction_counter;
            }

            if (not(mnemonic == "jump" or mnemonic == "if" or mnemonic == ".mark:")) {
                continue;
            }

            using viua::assembler::frontend::parser::Directive;
            using viua::assembler::frontend::parser::Instruction;
            if (mnemonic == "jump") {
                validate_jump(line->tokens.at(1), line->tokens.at(1), function_instruction_counter,
                              forward_jumps, deferred_marker_jumps, jump_targets);
            } else if (auto op = dynamic_cast<Instruction*>(line.get()); op and op->opcode == IF) {
                Token when_true = op->operands.at(1)->tokens.at(0);
                Token when_false = op->operands.at(2)->tokens.at(0);

                validate_jump(when_true, when_true, function_instruction_counter, forward_jumps,
                              deferred_marker_jumps, jump_targets);
                validate_jump(when_false, when_false, function_instruction_counter, forward_jumps,
                              deferred_marker_jumps, jump_targets);
                validate_jump_pair(line->tokens.at(0), when_true, when_false, function_instruction_counter,
                                   deferred_jump_pair_checks, jump_targets);
            } else if (auto mark = dynamic_cast<Directive*>(line.get());
                       mark and mark->directive == ".mark:") {
                jump_targets[mark->operands.at(0)] = function_instruction_counter;
            }
        }

        verify_forward_jumps(function_instruction_counter, forward_jumps);
        verify_marker_jumps(function_instruction_counter, deferred_marker_jumps, jump_targets);
        verify_deferred_jump_pairs(deferred_jump_pair_checks, jump_targets);
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
