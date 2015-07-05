#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <map>
#include <algorithm>
#include "../../bytecode/maps.h"
#include "../../support/string.h"
#include "../../program.h"
#include "assembler.h"
using namespace std;


string assembler::verify::functionCallsAreDefined(const vector<string>& lines, const vector<string>& function_names, const vector<string>& function_signatures) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not str::startswith(line, "call")) {
            continue;
        }

        line = str::lstrip(line.substr(4));
        string return_register = str::chunk(line);
        line = str::lstrip(line.substr(return_register.size()));
        string function = str::chunk(line);

        string& check_function = (function.size() ? function : return_register);

        // return register is optional to give
        // if it is not given - second operand is empty, and function name must be taken from first operand
        bool is_undefined = (find(function_names.begin(), function_names.end(), check_function) == function_names.end());

        if (is_undefined) {
            report << "fatal: call to undefined function '" << check_function << "' at line " << (i+1);
            break;
        }
    }
    return report.str();
}

string assembler::verify::frameBalance(const vector<string>& lines) {
    ostringstream report("");
    string line;
    string instruction;

    int balance = 0;
    int previous_frame_spawnline = 0;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (line.size() == 0) { continue; }

        line = str::lstrip(line);
        instruction = str::chunk(line);
        if (not (instruction == "call" or instruction == "excall" or instruction == "fcall" or instruction == "frame")) {
            continue;
        }

        if (instruction == "call" or instruction == "excall" or instruction == "fcall") {
            --balance;
        }
        if (instruction == "frame") {
            ++balance;
        }

        if (balance < 0) {
            report << "fatal: call with '" << instruction << "' without a frame at line " << (i+1);
            break;
        }
        if (balance > 1) {
            report << "fatal: excess frame spawned at line " << (i+1) << " (unused frame spawned at line " << (previous_frame_spawnline+1) << ')';
            break;
        }

        if (instruction == "frame") {
            previous_frame_spawnline = i;
        }
    }
    return report.str();
}

string assembler::verify::blockTries(const vector<string>& lines, const vector<string>& block_names) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not str::startswithchunk(line, "try")) {
            continue;
        }

        string block = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
        bool is_undefined = (find(block_names.begin(), block_names.end(), block) == block_names.end());

        if (is_undefined) {
            report << "fatal: try of undefined block '" << block << "' at line " << (i+1);
        }
    }
    return report.str();
}

string assembler::verify::callableCreations(const vector<string>& lines, const vector<string>& function_names, const vector<string>& function_signatures) {
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
            report << "fatal: " << callable_type << " from undefined function '" << function << "' at line " << (i+1);
            break;
        }

        // second chunk of closure instruction, must be an integer
        if (not str::isnum(register_index)) {
            report << "fatal: first operand (register index) is not an integer in " << callable_type << " instruction at line " << (i+1);
            break;
        }
    }
    return report.str();
}

string assembler::verify::ressInstructions(const vector<string>& lines, bool as_lib) {
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
        if (str::startswith(line, ".function:")) {
            function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            continue;
        }
        if (not str::startswith(line, "ress")) {
            continue;
        }

        string registerset_name = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));

        if (find(legal_register_sets.begin(), legal_register_sets.end(), registerset_name) == legal_register_sets.end()) {
            report << "fatal: illegal register set name in ress instruction: '" << registerset_name << "' at line " << (i+1);
            break;
        }
        if (registerset_name == "global" and as_lib and function != "main") {
            report << "fatal: global registers used in library function at line " << (i+1);
            break;
        }
    }
    return report.str();
}

string assembler::verify::functionBodiesAreNonempty(const vector<string>& lines, map<string, vector<string> >& functions) {
    ostringstream report("");
    string line;
    for (auto function : functions) {
        vector<string> flines = function.second;
        if (flines.size() == 0) {
            report << "fatal: function '" + function.first + "' is empty" << endl;
            break;
        }
    }
    return report.str();
}

string assembler::verify::directives(const vector<string>& lines) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (line.size() == 0 or line[0] != '.') {
            continue;
        }

        string token = str::chunk(line);
        if (not (token == ".function:" or token == ".signature:" or token == ".block:" or token == ".end" or token == ".name:" or token == ".mark:" or token == ".main:")) {
            report << "fatal: unrecognised assembler directive on line " << (i+1) << ": `" << token << '`';
            break;
        }
    }
    return report.str();
}
string assembler::verify::instructions(const vector<string>& lines) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (line.size() == 0 or line[0] == '.' or line[0] == ';') {
            continue;
        }

        string token = str::chunk(line);
        if (OP_SIZES.count(token) == 0) {
            report << "fatal: unrecognised instruction on line " << (i+1) << ": `" << token << '`';
            break;
        }
    }
    return report.str();
}
