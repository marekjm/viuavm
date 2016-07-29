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


string assembler::verify::functionCallsAreDefined(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines, const vector<string>& function_names, const vector<string>& function_signatures) {
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
            report << filename << ':' << (expanded_lines_to_source_lines.at(i)+1) << ": error: " << ((instr_name == "call" or instr_name == "tailcall") ? "call to" : instr_name == "process" ? "process from" : "watchdog from") << " undefined function " << check_function;
            break;
        }
    }
    return report.str();
}

string assembler::verify::functionCallArities(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines, bool) {
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
            report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: '" << function_name << "' is not a valid function name";
            break;
        }

        int arity = assembler::utils::getFunctionArity(function_name);

        if (arity == -1) {
            report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: call to function with undefined arity ";
            if (frame_parameters_count >= 0) {
                report << "as " << function_name << (arity == -1 ? "/" : "") << frame_parameters_count;
            } else {
                report << ": " << function_name;
            }
            break;
        }
        if (frame_parameters_count == -1) {
            // frame paramters count could not be statically determined, deffer the check until runtime
            continue;
        }

        if (arity > 0 and arity != frame_parameters_count) {
            report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: invalid number of parameters in call to function " << function_name << ": expected " << arity << " got " << frame_parameters_count;
            break;
        }
    }
    return report.str();
}

string assembler::verify::msgArities(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines, bool) {
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
            report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: '" << function_name << "' is not a valid function name";
            break;
        }

        if (frame_parameters_count == 0) {
            report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: invalid number of parameters in dynamic dispatch of " << function_name << ": expected at least 1, got 0";
            break;
        }

        int arity = assembler::utils::getFunctionArity(function_name);

        if (arity == -1) {
            report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: dynamic dispatch call with undefined arity ";
            if (frame_parameters_count >= 0) {
                report << "as " << function_name << (arity == -1 ? "/" : "") << frame_parameters_count;
            } else {
                report << ": " << function_name;
            }
            break;
        }
        if (frame_parameters_count == -1) {
            // frame paramters count could not be statically determined, deffer the check until runtime
            continue;
        }

        if (arity > 0 and arity != frame_parameters_count) {
            report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: invalid number of parameters in dynamic dispatch of " << function_name << ": expected " << arity << " got " << frame_parameters_count;
            break;
        }
    }
    return report.str();
}

string assembler::verify::functionNames(const string& filename, const std::vector<std::string>& lines, const bool, const bool) {
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
            report << filename << ':' << i+1 << ": error: invalid function name: " << function;
            break;
        }

        if (assembler::utils::getFunctionArity(function) == -1) {
            report << filename << ':' << i+1 << ": error: function with undefined arity: " << function;
            break;
        }
    }

    return report.str();
}

string assembler::verify::functionsEndWithReturn(const string& filename, const std::vector<std::string>& lines) {
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
            report << filename << ':' << i+1 << ": error: function does not end with 'return' or 'tailcall': " << function;
            break;
        }

        // if we're here, then the .end at the end of function has been reached and
        // the function name marker should be reset
        function = "";
    }

    return report.str();
}

string assembler::verify::frameBalance(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines) {
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
            report << filename << ':' << (expanded_lines_to_source_lines.at(i)+1) << ": error: call with '" << instruction << "' without a frame";
            break;
        }
        if (balance > 1) {
            report << filename << ':' << (expanded_lines_to_source_lines.at(i)+1) << ": error: excess frame spawned";
            report << " (unused frame spawned at line ";
            report << (expanded_lines_to_source_lines.at(previous_frame_spawnline)+1) << ')';
            break;
        }
        if (instruction == "return" and balance > 0) {
            report << filename << ':' << (expanded_lines_to_source_lines.at(i)+1) << ": error: leftover frame (spawned at line " << (expanded_lines_to_source_lines.at(previous_frame_spawnline)+1) << ')';
            break;
        }

        if (instruction == "frame") {
            previous_frame_spawnline = i;
        }
    }
    return report.str();
}

string assembler::verify::blockTries(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines, const vector<string>& block_names, const vector<string>& block_signatures) {
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
            report << filename << ':' << (expanded_lines_to_source_lines.at(i)+1) << ": error: cannot enter undefined block: " << block;
            break;
        }
    }
    return report.str();
}

string assembler::verify::blockCatches(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines, const vector<string>& block_names, const vector<string>& block_signatures) {
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
            report << filename << ':' << (expanded_lines_to_source_lines.at(i)+1) << ": error: cannot catch using undefined block: " << block;
            break;
        }
    }
    return report.str();
}

string assembler::verify::callableCreations(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines, const vector<string>& function_names, const vector<string>& function_signatures) {
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
            report << filename << ':' << (expanded_lines_to_source_lines.at(i)+1) << ": error: " << callable_type << " from undefined function: " << function;
            break;
        }
    }
    return report.str();
}

string assembler::verify::ressInstructions(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines, bool as_lib) {
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
            report << filename << ':' << (expanded_lines_to_source_lines.at(i)+1) << ": error: illegal register set name in ress instruction '" << registerset_name << "' in function " << function;
            break;
        }
        if (registerset_name == "global" and as_lib and function != "main/1") {
            report << filename << ':' << (expanded_lines_to_source_lines.at(i)+1) << ": error: global registers used in library function " << function;
            break;
        }
    }
    return report.str();
}

static string bodiesAreNonempty(const string& filename, const vector<string>& lines) {
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
            report << filename << ':' << i+1 << ": error: stray .end marker";
            break;
        }

        // this if is reached only of '.end' was matched - so we just check if it is preceded by
        // the interesting prefix, and
        // if it is - report an error
        if (i and (assembler::utils::lines::is_block(str::lstrip(lines[i-1])) or assembler::utils::lines::is_function(str::lstrip(lines[i-1])))) {
            report << filename << ':' << i << ": error: " << (function.size() ? "function" : "block") << " with empty body: " << (function.size() ? function : block);
            break;
        }
        function = "";
        block = "";
    }

    return report.str();
}
string assembler::verify::functionBodiesAreNonempty(const std::string& filename, const vector<string>& lines) {
    return bodiesAreNonempty(filename, lines);
}
string assembler::verify::blockBodiesAreNonempty(const string& filename, const std::vector<std::string>& lines) {
    return bodiesAreNonempty(filename, lines);
}

string assembler::verify::blocksEndWithFinishingInstruction(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines) {
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
            report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: missing returning instruction (leave, return or halt) at the end of block '" << block << "'";
            break;
        }

        // if we're here, then the .end at the end of function has been reached and
        // the block name marker should be reset
        block = "";
    }

    return report.str();
}

string assembler::verify::directives(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (line.size() == 0 or line[0] != '.') {
            continue;
        }

        if (not assembler::utils::lines::is_directive(line)) {
            report << filename << ':' << (expanded_lines_to_source_lines.at(i)+1) << ": error: illegal directive: '" << str::chunk(line) << "'";
            break;
        }
    }
    return report.str();
}
string assembler::verify::instructions(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (line.size() == 0 or line[0] == '.' or line[0] == ';' or str::startswith(line, "--")) {
            continue;
        }

        string token = str::chunk(line);
        if (OP_SIZES.count(token) == 0) {
            report << filename << ':' << (expanded_lines_to_source_lines.at(i)+1) << ": error: unknown instruction: '" << token << "'";
            break;
        }
    }
    return report.str();
}

string assembler::verify::framesHaveOperands(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines) {
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
            report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: frame instruction without operands";
            break;
        }
    }

    return report.str();
}

string assembler::verify::framesHaveNoGaps(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines) {
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
                report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: pass to parameter slot " << slot_index << " in frame with only " << frame_parameters_count << " slots available";
                break;
            }
            if (detected_slot_index and detected_frame_parameters_count) {
                if (filled_slots[slot_index]) {
                    report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: double pass to parameter slot " << slot_index << " in frame defined at line " << expanded_lines_to_source_lines.at(last_frame)+1;
                    report << ", first pass at line " << expanded_lines_to_source_lines.at(pass_lines[slot_index])+1;
                    break;
                }
                filled_slots[slot_index] = true;
                pass_lines[slot_index] = i;
            }
            continue;
        }

        if (instr_name == "call" or instr_name == "process" or instr_name == "watchdog") {
            for (decltype(frame_parameters_count) f = 0; f < frame_parameters_count; ++f) {
                if (not filled_slots[f]) {
                    report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: gap in frame defined at line " << expanded_lines_to_source_lines.at(last_frame)+1 << ", slot " << f << " left empty";
                    break;
                }
            }
        }
    }

    return report.str();
}

string assembler::verify::jumpsAreInRange(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines) {
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
                    report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: ";
                    report << "absolute jump with negative value";
                    break;
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
                report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: backward out-of-function jump";
                break;
            }
            if (target > function_instruction_counter) {
                forward_jumps.push_back({i, target});
            }
        } else if (first_part == ".mark:") {
            jump_targets[second_part] = function_instruction_counter;
            continue;
        } else if (first_part == ".end") {
            function_name = "";

            bool detected_out_of_range_jump = false;
            for (auto jmp : forward_jumps) {
                if (jmp.second > function_instruction_counter) {
                    report << filename << ':' << expanded_lines_to_source_lines.at(jmp.first)+1 << ": error: forward out-of-function jump";
                    detected_out_of_range_jump = true;
                    break;
                }
            }
            if (detected_out_of_range_jump) {
                break;
            }
            for (auto jmp : deferred_marker_jumps) {
                if (jump_targets.count(jmp.second) == 0) {
                    report << filename << ':' << expanded_lines_to_source_lines.at(jmp.first)+1 << ": error: jump to unrecognised marker: " << jmp.second;
                    detected_out_of_range_jump = true;
                    break;
                }
                if (jump_targets.at(jmp.second) > function_instruction_counter) {
                    report << filename << ':' << expanded_lines_to_source_lines.at(jmp.first)+1 << ": error: marker out-of-function jump";
                    detected_out_of_range_jump = true;
                    break;
                }
            }
            if (detected_out_of_range_jump) {
                break;
            }
        }
    }

    return report.str();
}
