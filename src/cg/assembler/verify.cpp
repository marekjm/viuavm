/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <map>
#include <set>
#include <algorithm>
#include <regex>
#include <viua/support/string.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/lex.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/program.h>
using namespace std;


using ErrorReport = pair<unsigned, string>;
using Token = viua::cg::lex::Token;


static bool is_defined(string function_name, const vector<string>& function_names, const vector<string>& function_signatures) {
    bool is_undefined = (find(function_names.begin(), function_names.end(), function_name) == function_names.end());
    // if function is undefined, check if we got a signature for it
    if (is_undefined) {
        is_undefined = (find(function_signatures.begin(), function_signatures.end(), function_name) == function_signatures.end());
    }
    return (not is_undefined);
}
void assembler::verify::functionCallsAreDefined(const vector<Token>& tokens, const vector<string>& function_names, const vector<string>& function_signatures) {
    ostringstream report("");
    string line;
    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        auto token = tokens.at(i);
        if (not (token == "call" or token == "process" or token == "watchdog" or token == "tailcall")) {
            continue;
        }

        if (token == "tailcall" or token == "watchdog") {
            string function_name = tokens.at(i+1);
            if (not is_defined(function_name, function_names, function_signatures)) {
                throw viua::cg::lex::InvalidSyntax(tokens.at(i+1), (string(token == "tailcall" ? "tail call to" : "watchdog from") + " undefined function " + function_name));
            }
        } else if (token == "call" or token == "process") {
            Token function_name = tokens.at(i+2);
            if (tokens.at(i+1) != "void") {
                function_name = tokens.at(i+3);
            }
            if (function_name.str().at(0) != '*' and function_name.str().at(0) != '%') {
                if (not is_defined(function_name, function_names, function_signatures)) {
                    throw viua::cg::lex::InvalidSyntax(function_name, (string(token == "call" ? "call to" : "process from") + " undefined function " + function_name.str()));
                }
            }
        }
    }
}

void assembler::verify::functionCallArities(const vector<Token>& tokens) {
    int frame_parameters_count = 0;
    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size(); ++i) {
        // watchdog function should be unary
        if (not (tokens.at(i) == "call" or tokens.at(i) == "process" /*or tokens.at(i) == "watchdog"*/ or tokens.at(i) == "frame")) {
            continue;
        }

        if (tokens.at(i) == "frame") {
            if (str::isnum(tokens.at(i+1).str().substr(1))) {
                frame_parameters_count = stoi(tokens.at(i+1).str().substr(1));
            } else {
                frame_parameters_count = -1;
            }
            continue;
        }

        Token function_name;
        if (tokens.at(i) == "call" or tokens.at(i) == "process") {
            function_name = tokens.at(i+2);
            if (tokens.at(i+1) != "void") {
                function_name = tokens.at(i+3);
            }
        } else {
            function_name = tokens.at(i+1);
        }

        if (not (function_name.str().at(0) == '*' or function_name.str().at(0) == '%' or assembler::utils::isValidFunctionName(function_name))) {
            throw viua::cg::lex::InvalidSyntax(function_name, ("not a valid function name: " + str::strencode(function_name)));
        }

        int arity = assembler::utils::getFunctionArity(function_name);

        if (function_name.str().at(0) == '*' or function_name.str().at(0) == '%') {
            // skip arity checks for functions called indirectly
            continue;
        }

        if (arity == -1) {
            ostringstream report;
            report << "call to function with undefined arity ";
            if (frame_parameters_count >= 0) {
                report << "as " << function_name.str() << (arity == -1 ? "/" : "") << frame_parameters_count;
            } else {
                report << ": " << function_name.str();
            }
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), report.str());
        }
        if (frame_parameters_count == -1) {
            // frame paramters count could not be statically determined, defer the check until runtime
            continue;
        }

        if (arity > 0 and arity != frame_parameters_count) {
            ostringstream report;
            report << "invalid number of parameters in call to function " << function_name.str() << ": expected " << arity << " got " << frame_parameters_count;
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), report.str());
        }
    }
}

void assembler::verify::msgArities(const vector<Token>& tokens) {
    int frame_parameters_count = 0;

    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size(); ++i) {
        if (not (tokens.at(i) == "msg" or tokens.at(i) == "frame")) {
            continue;
        }

        if (tokens.at(i) == "frame") {
            if (str::isnum(tokens.at(i+1).str().substr(1))) {
                frame_parameters_count = stoi(tokens.at(i+1).str().substr(1));
            } else {
                frame_parameters_count = -1;
            }
            continue;
        }

        Token function_name = tokens.at(i+3);
        if (tokens.at(i+1) == "void") {
            function_name = tokens.at(i+2);
        }

        if (not (function_name.str().at(0) == '*' or function_name.str().at(0) == '%' or assembler::utils::isValidFunctionName(function_name))) {
            throw viua::cg::lex::InvalidSyntax(function_name, ("not a valid function name: " + str::strencode(function_name)));
        }

        if (frame_parameters_count == 0) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("invalid number of parameters in dynamic dispatch of " + function_name.str() + ": expected at least 1, got 0"));
        }

        int arity = assembler::utils::getFunctionArity(function_name);

        if (arity == -1) {
            ostringstream report;
            report << "dynamic dispatch call with undefined arity ";
            if (frame_parameters_count >= 0) {
                report << "as " << function_name.str() << (arity == -1 ? "/" : "") << frame_parameters_count;
            } else {
                report << ": " << function_name.str();
            }
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), report.str());
        }
        if (frame_parameters_count == -1) {
            // frame paramters count could not be statically determined, deffer the check until runtime
            continue;
        }

        if (arity > 0 and arity != frame_parameters_count) {
            ostringstream report;
            report << "invalid number of parameters in dynamic dispatch of " << function_name.str() << ": expected " << arity << " got " << frame_parameters_count;
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), report.str());
        }
    }
}

void assembler::verify::functionNames(const vector<Token>& tokens) {
    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i) != ".function:") {
            continue;
        }

        string function = tokens.at(++i);

        if (not assembler::utils::isValidFunctionName(function)) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("invalid function name: " + function));
        }

        if (assembler::utils::getFunctionArity(function) == -1) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("function with undefined arity: " + function));
        }
    }
}

void assembler::verify::functionsEndWithReturn(const std::vector<Token>& tokens) {
    string function;

    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i) == ".function:") {
            function = tokens.at(i+1);
            continue;
        } else if (tokens.at(i) == ".end" and function.size()) {
            // .end may have been reached while not in a function because blocks also end with .end
            // so we also make sure that we were inside a function
        } else {
            continue;
        }

        bool last_token_returns = (tokens.at(i-1) == "return" or tokens.at(i-2) == "tailcall");
        bool last_but_one_token_returns = (tokens.at(i-1) == "\n" and (tokens.at(i-2) == "return" or tokens.at(i-3) == "tailcall"));
        if (not (last_token_returns or last_but_one_token_returns)) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("function does not end with 'return' or 'tailcall': " + function));
        }

        // if we're here, then the .end at the end of function has been reached and
        // the function name marker should be reset
        function = "";
    }
}

void assembler::verify::frameBalance(const vector<Token>& tokens) {
    string instruction;

    int balance = 0;
    Token previous_frame_spawned;
    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size(); ++i) {
        instruction = tokens.at(i);
        if (not (instruction == "call" or instruction == "tailcall" or instruction == "process" or instruction == "frame" or instruction == "msg" or
                 instruction == "return" or instruction == "leave" or instruction == "throw" or instruction == ".end")) {
            continue;
        }

        if (instruction == "call" or instruction == "tailcall" or instruction == "process" or instruction == "msg") {
            --balance;
        }
        if (instruction == "frame") {
            ++balance;
        }

        if (balance < 0) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("call with '" + instruction + "' without a frame"));
        }
        if (balance > 1) {
            throw viua::cg::lex::TracedSyntaxError()
                .append(viua::cg::lex::InvalidSyntax(tokens.at(i), "excess frame spawned"))
                .append(viua::cg::lex::InvalidSyntax(previous_frame_spawned, "unused frame:"))
                ;
        }
        if ((instruction == "return"  or instruction == "leave" or instruction == "throw" or instruction == ".end") and balance > 0) {
            throw viua::cg::lex::TracedSyntaxError()
                .append(viua::cg::lex::InvalidSyntax(tokens.at(i), "leftover frame:"))
                .append(viua::cg::lex::InvalidSyntax(previous_frame_spawned, "spawned here:"))
                ;
        }

        if (instruction == "frame") {
            previous_frame_spawned = tokens.at(i);
        }
    }
}

void assembler::verify::blockTries(const vector<Token>& tokens, const vector<string>& block_names, const vector<string>& block_signatures) {
    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i) != "enter") {
            continue;
        }

        string block = tokens.at(++i);
        bool is_undefined = (find(block_names.begin(), block_names.end(), block) == block_names.end());
        // if block is undefined, check if we got a signature for it
        if (is_undefined) {
            is_undefined = (find(block_signatures.begin(), block_signatures.end(), block) == block_signatures.end());
        }

        if (is_undefined) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("cannot enter undefined block: " + block));
        }
    }
}

void assembler::verify::blockCatches(const vector<Token>& tokens, const vector<string>& block_names, const vector<string>& block_signatures) {
    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i) != "catch") {
            continue;
        }

        // skip type name token
        ++i;
        string block = tokens.at(++i);

        bool is_undefined = (find(block_names.begin(), block_names.end(), block) == block_names.end());
        // if block is undefined, check if we got a signature for it
        if (is_undefined) {
            is_undefined = (find(block_signatures.begin(), block_signatures.end(), block) == block_signatures.end());
        }

        if (is_undefined) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("cannot catch using undefined block: " + block));
        }
    }
}

void assembler::verify::callableCreations(const vector<Token>& tokens, const vector<string>& function_names, const vector<string>& function_signatures) {
    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size(); ++i) {
        if (not (tokens.at(i) == "closure" or tokens.at(i) == "function")) {
            // skip while lines to avoid triggering the check by registers named 'function' or 'closure'
            while (i < tokens.size() and tokens.at(i) != "\n") {
                ++i;
            }
            continue;
        }

        string function = tokens.at(i+3);
        bool is_undefined = (find(function_names.begin(), function_names.end(), function) == function_names.end());
        // if function is undefined, check if we got a signature for it
        if (is_undefined) {
            is_undefined = (find(function_signatures.begin(), function_signatures.end(), function) == function_signatures.end());
        }

        if (is_undefined) {
            viua::cg::lex::InvalidSyntax e(tokens.at(i), (tokens.at(i).str() + " from undefined function: " + function));
            e.add(tokens.at(i+2));
            throw e;
        }
        i += 2;
    }
}

void assembler::verify::ressInstructions(const vector<Token>& tokens, bool as_lib) {
    ostringstream report("");
    vector<string> legal_register_sets = {
        "global",   // global register set
        "local",    // local register set for function
        "static",   // static register set
    };
    string function;
    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i) == ".function:") {
            function = tokens.at(i+1);
            continue;
        }
        if (tokens.at(i) != "ress") {
            continue;
        }

        string registerset_name = tokens.at(++i);

        if (find(legal_register_sets.begin(), legal_register_sets.end(), registerset_name) == legal_register_sets.end()) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("illegal register set name in ress instruction '" + registerset_name + "' in function " + function));
        }
        if (registerset_name == "global" and as_lib and function != "main/1") {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("global registers used in library function " + function));
        }
    }
}

static void bodiesAreNonempty(const vector<Token>& tokens) {
    ostringstream report("");
    Token block;
    bool function = false;

    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i) == ".block:") {
            block = tokens.at(i);
            function = false;
            continue;
        } else if (tokens.at(i) == ".function:") {
            block = tokens.at(i);
            function = true;
            continue;
        } else if (tokens.at(i) == ".closure:") {
            block = tokens.at(i);
            function = true;
            continue;
        } else if (tokens.at(i) == ".end") {
            // '.end' is also interesting because we want to see if it's immediately preceded by
            // the interesting prefix
        } else {
            continue;
        }

        if (not block.str().size()) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), "stray .end marker");
        }

        // this 'if' is reached only if '.end' was matched - so we just check if it is preceded by
        // the interesting prefix, and
        // if it is - report an error
        if (i and (tokens.at(i-3) == ".function:" or tokens.at(i-3) == ".block:" or tokens.at(i-3) == ".closure:")) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i-2), (string(function ? "function" : "block") + " with empty body: " + tokens.at(i-2).str()));
        }
        function = false;
        block.str("");
    }
}
void assembler::verify::functionBodiesAreNonempty(const vector<Token>& tokens) {
    bodiesAreNonempty(tokens);
}
void assembler::verify::blockBodiesAreNonempty(const vector<Token>& tokens) {
    bodiesAreNonempty(tokens);
}

void assembler::verify::blocksEndWithFinishingInstruction(const vector<Token>& tokens) {
    string block;

    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size(); ++i) {
        if (tokens.at(i) == ".block:") {
            block = tokens.at(i+1);
            continue;
        } else if (tokens.at(i) == ".end" and block.size()) {
            // .end may have been reached while not in a block because functions also end with .end
            // so we also make sure that we were inside a block
        } else {
            continue;
        }

        bool last_token_returns = (tokens.at(i-1) == "leave" or tokens.at(i-1) == "return" or tokens.at(i-1) == "tailcall" or tokens.at(i-1) == "halt");
        bool last_but_one_token_returns = (tokens.at(i-1) == "\n" and (tokens.at(i-2) == "leave" or tokens.at(i-2) == "return" or tokens.at(i-2) == "tailcall" or tokens.at(i-2) == "halt"));
        if (not (last_token_returns or last_but_one_token_returns)) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("missing returning instruction (leave, return, tailcall or halt) at the end of block: " + block));
        }

        // if we're here, then the .end at the end of function has been reached and
        // the block name marker should be reset
        block = "";
    }
}

void assembler::verify::directives(const vector<Token>& tokens) {
    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        if (not (i == 0 or tokens.at(i-1) == "\n")) {
            continue;
        }
        if (tokens.at(i).str().at(0) != '.') {
            continue;
        }
        if (not assembler::utils::lines::is_directive(tokens.at(i))) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), "illegal directive");
        }
    }
}

void assembler::verify::instructions(const vector<Token>& tokens) {
    for (decltype(tokens.size()) i = 1; i < tokens.size(); ++i) {
        // instructions can only appear after newline
        if (tokens.at(i-1) != "\n") {
            continue;
        }
        // directives and newlines can *also* apear after newline so filter them out
        if (tokens.at(i).str().at(0) == '.' or tokens.at(i) == "\n") {
            continue;
        }
        if (OP_MNEMONICS.count(tokens.at(i)) == 0) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("unknown instruction: '" + tokens.at(i).str() + "'"));
        }
    }
}

void assembler::verify::framesHaveNoGaps(const vector<Token>& tokens) {
    unsigned long frame_parameters_count = 0;
    bool detected_frame_parameters_count = false;
    bool slot_index_detection_is_reliable = true;
    unsigned long last_frame = 0;

    vector<bool> filled_slots;
    vector<unsigned long> pass_lines;

    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size(); ++i) {
        if (not (tokens.at(i) == "call" or tokens.at(i) == "process" or tokens.at(i) == "frame" or tokens.at(i) == "param" or tokens.at(i) == "pamv")) {
            continue;
        }

        if (tokens.at(i) == "frame") {
            last_frame = tokens.at(i).line();

            if (str::isnum(tokens.at(i+1).str().substr(1))) {
                frame_parameters_count = stoul(tokens.at(i+1).str().substr(1));
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

        if (tokens.at(i) == "param" or tokens.at(i) == "pamv") {
            unsigned long slot_index;
            bool detected_slot_index = false;
            if (tokens.at(i+1).str().at(0) == '@') {
                slot_index_detection_is_reliable = false;
            }
            if (tokens.at(i+1).str().at(0) == '%' and str::isnum(tokens.at(i+1).str().substr(1))) {
                slot_index = stoul(tokens.at(i+1).str().substr(1));
                detected_slot_index = true;
            }
            if (detected_slot_index and detected_frame_parameters_count and slot_index >= frame_parameters_count) {
                ostringstream report;
                report << "pass to parameter slot " << slot_index << " in frame with only " << frame_parameters_count << " slots available";
                throw viua::cg::lex::InvalidSyntax(tokens.at(i), report.str());
            }
            if (detected_slot_index and detected_frame_parameters_count) {
                if (filled_slots[slot_index]) {
                    ostringstream report;
                    report << "double pass to parameter slot " << slot_index << " in frame defined at line " << last_frame+1;
                    report << ", first pass at line " << pass_lines[slot_index]+1;
                    throw viua::cg::lex::InvalidSyntax(tokens.at(i), report.str());
                }
                filled_slots[slot_index] = true;
                pass_lines[slot_index] = tokens.at(i).line();
            }
            continue;
        }

        if (slot_index_detection_is_reliable and (tokens.at(i) == "call" or tokens.at(i) == "process")) {
            for (decltype(frame_parameters_count) f = 0; f < frame_parameters_count; ++f) {
                if (not filled_slots[f]) {
                    ostringstream report;
                    report << "gap in frame defined at line " << last_frame+1 << ", slot " << f << " left empty";
                    throw viua::cg::lex::InvalidSyntax(tokens.at(i), report.str());
                }
            }
        }
    }
}

static void validate_jump(const Token token, const string& extracted_jump, const int function_instruction_counter, vector<pair<Token, int>>& forward_jumps, vector<pair<Token, string>>& deferred_marker_jumps, const map<string, int>& jump_targets) {
    int target = -1;
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
            target = jump_targets.at(extracted_jump)+1;
        }
    }

    if (target < 0) {
        throw viua::cg::lex::InvalidSyntax(token, "backward out-of-range jump");
    }
    if (target == function_instruction_counter) {
        throw viua::cg::lex::InvalidSyntax(token, "zero-distance jump");
    }
    if (target > function_instruction_counter) {
        forward_jumps.emplace_back(token, target);
    }
}

static void validate_jump_pair(
    const Token& branch_token,
    const Token& when_true,
    const Token& when_false,
    int function_instruction_counter,
    vector<tuple<Token, Token, Token, int>>& deferred_jump_pair_checks,
    const map<string, int>& jump_targets
) {
    if (when_true.str() == when_false.str()) {
        throw viua::cg::lex::InvalidSyntax(branch_token, "useless branch: both targets point to the same instruction");
    }

    int true_target = 0, false_target = 0;
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
            deferred_jump_pair_checks.emplace_back(branch_token, when_true, when_false, function_instruction_counter);
            return;
        } else {
            // FIXME: jump targets are saved with an off-by-one error, that surfaces when
            // a .mark: directive immediately follows .function: declaration
            true_target = jump_targets.at(when_true)+1;
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
            deferred_jump_pair_checks.emplace_back(branch_token, when_true, when_false, function_instruction_counter);
            return;
        } else {
            // FIXME: jump targets are saved with an off-by-one error, that surfaces when
            // a .mark: directive immediately follows .function: declaration
            false_target = jump_targets.at(when_false)+1;
        }
    }

    if (true_target == false_target) {
        throw viua::cg::lex::InvalidSyntax(branch_token, "useless branch: both targets point to the same instruction");
    }
}

static void verify_forward_jumps(const int function_instruction_counter, const vector<pair<Token, int>>& forward_jumps) {
    for (auto jmp : forward_jumps) {
        if (jmp.second > function_instruction_counter) {
            throw viua::cg::lex::InvalidSyntax(jmp.first, "forward out-of-range jump");
        }
    }
}

static void verify_marker_jumps(const int function_instruction_counter, const vector<pair<Token, string>>& deferred_marker_jumps, const map<string, int>& jump_targets) {
    for (auto jmp : deferred_marker_jumps) {
        if (jump_targets.count(jmp.second) == 0) {
            throw viua::cg::lex::InvalidSyntax(jmp.first, ("jump to unrecognised marker: " + jmp.second));
        }
        if (jump_targets.at(jmp.second) > function_instruction_counter) {
            throw viua::cg::lex::InvalidSyntax(jmp.first, "marker out-of-range jump");
        }
    }
}

static void verify_deferred_jump_pairs(const vector<tuple<Token, Token, Token, int>>& deferred_jump_pair_checks, const map<string, int>& jump_targets) {
    for (const auto& each : deferred_jump_pair_checks) {
        int function_instruction_counter = 0;
        Token branch_token, when_true, when_false;
        tie(branch_token, when_true, when_false, function_instruction_counter) = each;

        int true_target = 0, false_target = 0;
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
            true_target = jump_targets.at(when_true)+1;
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
            false_target = jump_targets.at(when_false)+1;
        }

        if (true_target == false_target) {
            throw viua::cg::lex::InvalidSyntax(branch_token, "useless branch: both targets point to the same instruction");
        }
    }
}

static auto skip_till_next_line(const std::vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> decltype(i) {
    do {
        ++i;
    } while (i < tokens.size() and tokens.at(i) != "\n");
    return i;
}
void assembler::verify::jumpsAreInRange(const vector<viua::cg::lex::Token>& tokens) {
    map<string, int> jump_targets;
    vector<pair<Token, int>> forward_jumps;
    vector<pair<Token, string>> deferred_marker_jumps;
    vector<tuple<Token, Token, Token, int>> deferred_jump_pair_checks;
    int function_instruction_counter = 0;
    string function_name;

    for (std::remove_reference<decltype(tokens)>::type::size_type i = 0; i < tokens.size(); ++i) {
        if ((not function_name.empty()) and not str::startswith(tokens.at(i), ".")) {
            ++function_instruction_counter;
        }

        string mnemonic = tokens.at(i);
        if (not (mnemonic == ".function:" or mnemonic == ".closure:" or mnemonic == ".block:" or mnemonic == "jump" or mnemonic == "if" or mnemonic == ".mark:" or mnemonic == ".end")) {
            i = skip_till_next_line(tokens, i);
            continue;
        }

        if (mnemonic == ".function:" or mnemonic ==".closure:" or mnemonic == ".block:") {
            function_instruction_counter = -1;
            jump_targets.clear();
            forward_jumps.clear();
            deferred_marker_jumps.clear();
            deferred_jump_pair_checks.clear();
            function_name = tokens.at(i+1);
        } else if (mnemonic == "jump") {
            validate_jump(tokens.at(i+1), tokens.at(i+1), function_instruction_counter, forward_jumps, deferred_marker_jumps, jump_targets);
        } else if (mnemonic == "if") {
            Token when_true = tokens.at(i+2);
            Token when_false = tokens.at(i+3);

            validate_jump(when_true, when_true, function_instruction_counter, forward_jumps, deferred_marker_jumps, jump_targets);
            validate_jump(when_false, when_false, function_instruction_counter, forward_jumps, deferred_marker_jumps, jump_targets);
            validate_jump_pair(tokens.at(i), when_true, when_false, function_instruction_counter, deferred_jump_pair_checks, jump_targets);
        } else if (mnemonic == ".mark:") {
            jump_targets[tokens.at(i+1)] = function_instruction_counter;
        } else if (mnemonic == ".end") {
            function_name = "";
            verify_forward_jumps(function_instruction_counter, forward_jumps);
            verify_marker_jumps(function_instruction_counter, deferred_marker_jumps, jump_targets);
            verify_deferred_jump_pairs(deferred_jump_pair_checks, jump_targets);
        }

        i = skip_till_next_line(tokens, i);
    }
}
