/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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
#include <algorithm>
#include <regex>
#include <viua/support/string.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/program.h>
using namespace std;


using ErrorReport = pair<unsigned, string>;


void assembler::verify::functionCallsAreDefined(const vector<string>& lines, const vector<string>& function_names, const vector<string>& function_signatures) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not (str::startswith(line, "call") or str::startswith(line, "process") or str::startswith(line, "watchdog"))) {
            continue;
        }

        string instr_name = str::chunk(line);
        line = str::lstrip(line.substr(instr_name.size()));
        string return_register = str::chunk(line);
        line = str::lstrip(line.substr(return_register.size()));
        string function = str::chunk(line);

        // return register is optional to give
        // if it is not given - second operand is empty, and function name must be taken from first operand
        string& check_function = (function.size() ? function : return_register);

        bool is_undefined = (find(function_names.begin(), function_names.end(), check_function) == function_names.end());
        // if function is undefined, check if we got a signature for it
        if (is_undefined) {
            is_undefined = (find(function_signatures.begin(), function_signatures.end(), check_function) == function_signatures.end());
        }

        if (is_undefined) {
            report << ((instr_name == "call" or instr_name == "tailcall") ? "call to" : instr_name == "process" ? "process from" : "watchdog from") << " undefined function " << check_function;
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::functionCallArities(const vector<string>& lines) {
    ostringstream report("");
    string line;
    int frame_parameters_count = 0;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not (str::startswith(line, "call") or str::startswith(line, "process") or str::startswith(line, "watchdog") or str::startswith(line, "frame"))) {
            continue;
        }

        string instr_name = str::chunk(line);
        line = str::lstrip(line.substr(instr_name.size()));

        if (instr_name == "frame") {
            line = str::chunk(line);
            if (str::isnum(line)) {
                frame_parameters_count = stoi(str::chunk(line));
            } else {
                frame_parameters_count = -1;
            }
            continue;
        }

        string function_name = str::chunk(line);
        line = str::lstrip(line.substr(function_name.size()));
        if (line.size()) {
            function_name = str::chunk(line);
        }

        if (not assembler::utils::isValidFunctionName(function_name)) {
            report << "'" << function_name << "' is not a valid function name";
            throw ErrorReport(i, report.str());
        }

        int arity = assembler::utils::getFunctionArity(function_name);

        if (arity == -1) {
            report << "call to function with undefined arity ";
            if (frame_parameters_count >= 0) {
                report << "as " << function_name << (arity == -1 ? "/" : "") << frame_parameters_count;
            } else {
                report << ": " << function_name;
            }
            throw ErrorReport(i, report.str());
        }
        if (frame_parameters_count == -1) {
            // frame paramters count could not be statically determined, deffer the check until runtime
            continue;
        }

        if (arity > 0 and arity != frame_parameters_count) {
            report << "invalid number of parameters in call to function " << function_name << ": expected " << arity << " got " << frame_parameters_count;
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::msgArities(const vector<string>& lines) {
    ostringstream report("");
    string line;
    int frame_parameters_count = 0;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not (str::startswith(line, "msg") or str::startswith(line, "frame"))) {
            continue;
        }

        string instr_name = str::chunk(line);
        line = str::lstrip(line.substr(instr_name.size()));

        if (instr_name == "frame") {
            line = str::chunk(line);
            if (str::isnum(line)) {
                frame_parameters_count = stoi(str::chunk(line));
            } else {
                frame_parameters_count = -1;
            }
            continue;
        }

        string function_name = str::chunk(line);
        if (str::isnum(function_name)) {
            function_name = str::chunk(str::lstrip(line.substr(function_name.size())));
        }

        if (not assembler::utils::isValidFunctionName(function_name)) {
            report << "'" << function_name << "' is not a valid function name";
            throw ErrorReport(i, report.str());
        }

        if (frame_parameters_count == 0) {
            report << "invalid number of parameters in dynamic dispatch of " << function_name << ": expected at least 1, got 0";
            throw ErrorReport(i, report.str());
        }

        int arity = assembler::utils::getFunctionArity(function_name);

        if (arity == -1) {
            report << "dynamic dispatch call with undefined arity ";
            if (frame_parameters_count >= 0) {
                report << "as " << function_name << (arity == -1 ? "/" : "") << frame_parameters_count;
            } else {
                report << ": " << function_name;
            }
            throw ErrorReport(i, report.str());
        }
        if (frame_parameters_count == -1) {
            // frame paramters count could not be statically determined, deffer the check until runtime
            continue;
        }

        if (arity > 0 and arity != frame_parameters_count) {
            report << "invalid number of parameters in dynamic dispatch of " << function_name << ": expected " << arity << " got " << frame_parameters_count;
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::functionNames(const std::vector<std::string>& lines) {
    ostringstream report("");
    string line;
    string function;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (assembler::utils::lines::is_function(line)) {
            function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
        } else {
            continue;
        }

        if (not assembler::utils::isValidFunctionName(function)) {
            report << "invalid function name: " << function;
            throw ErrorReport(i, report.str());
        }

        if (assembler::utils::getFunctionArity(function) == -1) {
            report << "function with undefined arity: " << function;
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::functionsEndWithReturn(const std::vector<std::string>& lines) {
    ostringstream report("");
    string line;
    string function;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (assembler::utils::lines::is_function(line)) {
            function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            continue;
        } else if (str::startswithchunk(line, ".end") and function.size()) {
            // .end may have been reached while not in a function because blocks also end with .end
            // so we also make sure that we were inside a function
        } else {
            continue;
        }

        if (i and (not (str::startswithchunk(str::lstrip(lines[i-1]), "return") or str::startswithchunk(str::lstrip(lines[i-1]), "tailcall")))) {
            report << "function does not end with 'return' or 'tailcall': " << function;
            throw ErrorReport(i, report.str());
        }

        // if we're here, then the .end at the end of function has been reached and
        // the function name marker should be reset
        function = "";
    }
}

void assembler::verify::frameBalance(const vector<string>& lines, const map<unsigned long, unsigned long>& expanded_lines_to_source_lines) {
    ostringstream report("");
    string line;
    string instruction;

    int balance = 0;
    long unsigned previous_frame_spawnline = 0;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (line.size() == 0) { continue; }

        line = str::lstrip(line);
        instruction = str::chunk(line);
        if (not (instruction == "call" or instruction == "tailcall" or instruction == "process" or instruction == "watchdog" or instruction == "fcall" or instruction == "frame" or instruction == "msg" or instruction == "return")) {
            continue;
        }

        if (instruction == "call" or instruction == "tailcall" or instruction == "process" or instruction == "fcall" or instruction == "msg" or instruction == "watchdog") {
            --balance;
        }
        if (instruction == "frame") {
            ++balance;
        }

        if (balance < 0) {
            report << "call with '" << instruction << "' without a frame";
            throw ErrorReport(i, report.str());
        }
        if (balance > 1) {
            report << "excess frame spawned (unused frame spawned at line ";
            report << (expanded_lines_to_source_lines.at(previous_frame_spawnline)+1) << ')';
            throw ErrorReport(i, report.str());
        }
        if (instruction == "return" and balance > 0) {
            report << "leftover frame (spawned at line " << (expanded_lines_to_source_lines.at(previous_frame_spawnline)+1) << ')';
            throw ErrorReport(i, report.str());
        }

        if (instruction == "frame") {
            previous_frame_spawnline = i;
        }
    }
}

void assembler::verify::blockTries(const vector<string>& lines, const vector<string>& block_names, const vector<string>& block_signatures) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not str::startswithchunk(line, "enter")) {
            continue;
        }

        string block = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
        bool is_undefined = (find(block_names.begin(), block_names.end(), block) == block_names.end());
        // if block is undefined, check if we got a signature for it
        if (is_undefined) {
            is_undefined = (find(block_signatures.begin(), block_signatures.end(), block) == block_signatures.end());
        }

        if (is_undefined) {
            report << "cannot enter undefined block: " << block;
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::blockCatches(const vector<string>& lines, const vector<string>& block_names, const vector<string>& block_signatures) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not str::startswithchunk(line, "catch")) {
            continue;
        }

        // remove instruction
        line = str::lstrip(str::sub(line, str::chunk(line).size()));

        // remove name of caught type
        line = str::lstrip(str::sub(line, str::chunk(line).size()));

        string block = str::chunk(line);
        bool is_undefined = (find(block_names.begin(), block_names.end(), block) == block_names.end());
        // if block is undefined, check if we got a signature for it
        if (is_undefined) {
            is_undefined = (find(block_signatures.begin(), block_signatures.end(), block) == block_signatures.end());
        }

        if (is_undefined) {
            report << "cannot catch using undefined block: " << block;
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::callableCreations(const vector<string>& lines, const vector<string>& function_names, const vector<string>& function_signatures) {
    ostringstream report("");
    string line;
    string callable_type;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not str::startswith(line, "closure") and not str::startswith(line, "function")) {
            continue;
        }

        callable_type = str::chunk(line);
        line = str::lstrip(str::sub(line, callable_type.size()));

        string register_index = str::chunk(line);
        line = str::lstrip(str::sub(line, register_index.size()));

        string function = str::chunk(line);
        bool is_undefined = (find(function_names.begin(), function_names.end(), function) == function_names.end());
        // if function is undefined, check if we got a signature for it
        if (is_undefined) {
            is_undefined = (find(function_signatures.begin(), function_signatures.end(), function) == function_signatures.end());
        }

        if (is_undefined) {
            report << callable_type << " from undefined function: " << function;
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::ressInstructions(const vector<string>& lines, bool as_lib) {
    ostringstream report("");
    vector<string> legal_register_sets = {
        "global",   // global register set
        "local",    // local register set for function
        "static",   // static register set
        "temp",     // temporary register set
    };
    string line;
    string function;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (assembler::utils::lines::is_function(line)) {
            function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            continue;
        }
        if (not str::startswith(line, "ress")) {
            continue;
        }

        string registerset_name = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));

        if (find(legal_register_sets.begin(), legal_register_sets.end(), registerset_name) == legal_register_sets.end()) {
            report << "illegal register set name in ress instruction '" << registerset_name << "' in function " << function;
            throw ErrorReport(i, report.str());
        }
        if (registerset_name == "global" and as_lib and function != "main/1") {
            report << "global registers used in library function " << function;
            throw ErrorReport(i, report.str());
        }
    }
}

static void bodiesAreNonempty(const vector<string>& lines) {
    ostringstream report("");
    string line, block, function;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (assembler::utils::lines::is_block(line)) {
            block = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            continue;
        } else if (assembler::utils::lines::is_function(line)) {
            function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            continue;
        } else if (assembler::utils::lines::is_end(line)) {
            // '.end' is also interesting because we want to see if it's immediately preceded by
            // the interesting prefix
        } else {
            continue;
        }

        if (not (block.size() or function.size())) {
            report << "stray .end marker";
            throw ErrorReport(i, report.str());
        }

        // this if is reached only of '.end' was matched - so we just check if it is preceded by
        // the interesting prefix, and
        // if it is - report an error
        if (i and (assembler::utils::lines::is_block(str::lstrip(lines[i-1])) or assembler::utils::lines::is_function(str::lstrip(lines[i-1])))) {
            report << (function.size() ? "function" : "block") << " with empty body: " << (function.size() ? function : block);
            throw ErrorReport(i, report.str());
        }
        function = "";
        block = "";
    }
}
void assembler::verify::functionBodiesAreNonempty(const vector<string>& lines) {
    bodiesAreNonempty(lines);
}
void assembler::verify::blockBodiesAreNonempty(const std::vector<std::string>& lines) {
    bodiesAreNonempty(lines);
}

void assembler::verify::blocksEndWithFinishingInstruction(const vector<string>& lines) {
    ostringstream report("");
    string line;
    string block;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (assembler::utils::lines::is_block(line)) {
            block = str::chunk(str::lstrip(str::sub(line, string(".block:").size())));
            continue;
        } else if (str::startswithchunk(line, ".end") and block.size()) {
            // .end may have been reached while not in a block because functions also end with .end
            // so we also make sure that we were inside a block
        } else {
            continue;
        }

        if (i and (not (str::startswithchunk(str::lstrip(lines[i-1]), "leave") or str::startswithchunk(str::lstrip(lines[i-1]), "return") or str::startswithchunk(str::lstrip(lines[i-1]), "halt")))) {
            report << "missing returning instruction (leave, return or halt) at the end of block '" << block << "'";
            throw ErrorReport(i, report.str());
        }

        // if we're here, then the .end at the end of function has been reached and
        // the block name marker should be reset
        block = "";
    }
}

void assembler::verify::directives(const vector<string>& lines) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (line.size() == 0 or line[0] != '.') {
            continue;
        }

        if (not assembler::utils::lines::is_directive(line)) {
            report << "illegal directive: '" << str::chunk(line) << "'";
            throw ErrorReport(i, report.str());
        }
    }
}
void assembler::verify::instructions(const vector<string>& lines) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (line.size() == 0 or line[0] == '.' or line[0] == ';' or str::startswith(line, "--")) {
            continue;
        }

        string token = str::chunk(line);
        if (OP_SIZES.count(token) == 0) {
            report << "unknown instruction: '" << token << "'";
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::framesHaveOperands(const vector<string>& lines) {
    ostringstream report("");
    string line;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);

        if (not str::startswith(line, "frame")) {
            continue;
        }

        // remove leading instruction name and
        // strip the remaining string of leading whitespace
        line = str::lstrip(str::sub(line, str::chunk(line).size()));

        if (line.size() == 0) {
            report << "frame instruction without operands";
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::framesHaveNoGaps(const vector<string>& lines, const map<unsigned long, unsigned long>& expanded_lines_to_source_lines) {
    ostringstream report("");
    string line;
    unsigned long frame_parameters_count = 0;
    bool detected_frame_parameters_count = false;
    unsigned last_frame = 0;

    vector<bool> filled_slots;
    vector<unsigned> pass_lines;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not (str::startswith(line, "call") or str::startswith(line, "process") or str::startswith(line, "watchdog") or str::startswith(line, "frame") or str::startswith(line, "param") or str::startswith(line, "pamv"))) {
            continue;
        }

        string instr_name = str::chunk(line);
        line = str::lstrip(line.substr(instr_name.size()));

        if (instr_name == "frame") {
            last_frame = i;

            line = str::chunk(line);
            if (str::isnum(line)) {
                frame_parameters_count = stoul(str::chunk(line));
                filled_slots.clear();
                pass_lines.clear();
                filled_slots.resize(frame_parameters_count, false);
                pass_lines.resize(frame_parameters_count);
                detected_frame_parameters_count = true;
            } else {
                detected_frame_parameters_count = false;
            }
            continue;
        }

        if (instr_name == "param" or instr_name == "pamv") {
            line = str::chunk(line);
            unsigned long slot_index;
            bool detected_slot_index = false;
            if (str::isnum(line)) {
                slot_index = stoul(line);
                detected_slot_index = true;
            }
            if (detected_slot_index and detected_frame_parameters_count and slot_index >= frame_parameters_count) {
                report << "pass to parameter slot " << slot_index << " in frame with only " << frame_parameters_count << " slots available";
                throw ErrorReport(i, report.str());
            }
            if (detected_slot_index and detected_frame_parameters_count) {
                if (filled_slots[slot_index]) {
                    report << "double pass to parameter slot " << slot_index << " in frame defined at line " << expanded_lines_to_source_lines.at(last_frame)+1;
                    report << ", first pass at line " << expanded_lines_to_source_lines.at(pass_lines[slot_index])+1;
                    throw ErrorReport(i, report.str());
                }
                filled_slots[slot_index] = true;
                pass_lines[slot_index] = i;
            }
            continue;
        }

        if (instr_name == "call" or instr_name == "process" or instr_name == "watchdog") {
            for (decltype(frame_parameters_count) f = 0; f < frame_parameters_count; ++f) {
                if (not filled_slots[f]) {
                    report << "gap in frame defined at line " << expanded_lines_to_source_lines.at(last_frame)+1 << ", slot " << f << " left empty";
                    throw ErrorReport(i, report.str());
                }
            }
        }
    }
}

void assembler::verify::jumpsAreInRange(const vector<string>& lines) {
    ostringstream report("");
    string line;

    map<string, int> jump_targets;
    vector<pair<unsigned, int>> forward_jumps;
    vector<pair<unsigned, string>> deferred_marker_jumps;
    int function_instruction_counter = 0;
    string function_name;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);

        if (line.size() == 0) {
            continue;
        }
        if (function_name.size() > 0 and not str::startswith(line, ".")) {
            ++function_instruction_counter;
        }
        if (not (str::startswith(line, ".function:") or str::startswith(line, "jump") or str::startswith(line, ".mark:") or str::startswith(line, ".end"))) {
            continue;
        }

        string first_part = str::chunk(line);
        line = str::lstrip(line.substr(first_part.size()));

        string second_part = str::chunk(line);
        line = str::lstrip(line.substr(second_part.size()));

        if (first_part == ".function:") {
            function_instruction_counter = -1;
            jump_targets.clear();
            forward_jumps.clear();
            deferred_marker_jumps.clear();
            function_name = second_part;
            continue;
        } else if (first_part == "jump") {
            int target = -1;
            if (str::isnum(second_part)) {
                target = stoi(second_part);
            } else if (str::startswith(second_part, "+") and str::isnum(second_part.substr(1))) {
                target = (function_instruction_counter + stoi(second_part.substr(1)));
            } else if (str::startswith(second_part, ".") and str::isnum(second_part.substr(1))) {
                if (stoi(second_part.substr(1)) < 0) {
                    throw ErrorReport(i, "absolute jump with negative value");
                }
                // absolute jumps cannot be verified without knowing how many bytes the bytecode spans
                // this is a FIXME: add check for absolute jumps
                continue;
            } else if (str::ishex(second_part)) {
                // absolute jumps cannot be verified without knowing how many bytes the bytecode spans
                // this is a FIXME: add check for absolute jumps
                continue;
            } else {
                if (jump_targets.count(second_part) == 0) {
                    deferred_marker_jumps.push_back({i, second_part});
                    continue;
                } else {
                    target = jump_targets.at(second_part);
                }
            }

            if (target < 0 and (function_instruction_counter+target) < 0) {
                throw ErrorReport(i, "backward out-of-function jump");
            }
            if (target > function_instruction_counter) {
                forward_jumps.push_back({i, target});
            }
        } else if (first_part == ".mark:") {
            jump_targets[second_part] = function_instruction_counter;
            continue;
        } else if (first_part == ".end") {
            function_name = "";

            for (auto jmp : forward_jumps) {
                if (jmp.second > function_instruction_counter) {
                    report << "forward out-of-function jump";
                    throw ErrorReport(jmp.first, report.str());
                }
            }
            for (auto jmp : deferred_marker_jumps) {
                if (jump_targets.count(jmp.second) == 0) {
                    report << "jump to unrecognised marker: " << jmp.second;
                    throw ErrorReport(jmp.first, report.str());
                }
                if (jump_targets.at(jmp.second) > function_instruction_counter) {
                    report << "marker out-of-function jump";
                    throw ErrorReport(jmp.first, report.str());
                }
            }
        }
    }
}
