#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <map>
#include <algorithm>
#include "../support/string.h"
#include "../program.h"
#include "assembler.h"
using namespace std;


string assembler::verify::functionCalls(const vector<string>& lines, const vector<string>& function_names) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not str::startswith(line, "call")) {
            continue;
        }

        string function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
        bool is_undefined = (find(function_names.begin(), function_names.end(), function) == function_names.end());

        if (is_undefined) {
            report << "fatal: call to undefined function '" << function << "' at line " << i;
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
            report << "fatal: try of undefined block '" << block << "' at line " << i;
        }
    }
    return report.str();
}

string assembler::verify::callableCreations(const vector<string>& lines, const vector<string>& function_names) {
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
        string function = str::chunk(line);
        bool is_undefined = (find(function_names.begin(), function_names.end(), function) == function_names.end());

        if (is_undefined) {
            report << "fatal: " << callable_type << " from undefined function '" << function << "' at line " << i;
            break;
        }

        // second chunk of closure instruction, must be an integer
        line = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
        if (line.size() == 0) {
            report << "fatal: second operand missing from " << callable_type << " instruction at line " << i;
            break;
        } else if (not str::isnum(line)) {
            report << "fatal: second operand is not an integer in " << callable_type << " instruction at line " << i;
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
        if (str::startswith(line, ".def:")) {
            function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            continue;
        }
        if (not str::startswith(line, "ress")) {
            continue;
        }

        string registerset_name = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));

        if (find(legal_register_sets.begin(), legal_register_sets.end(), registerset_name) == legal_register_sets.end()) {
            report << "fatal: illegal register set name in ress instruction: '" << registerset_name << "' at line " << i;
            break;
        }
        if (registerset_name == "global" and as_lib and function != "main") {
            report << "fatal: global registers used in library function at line " << i;
            break;
        }
    }
    return report.str();
}

string assembler::verify::functionBodiesAreNonempty(const vector<string>& lines, map<string, pair<bool, vector<string> > >& functions) {
    ostringstream report("");
    string line;
    for (auto function : functions) {
        vector<string> flines = function.second.second;
        if (flines.size() == 0) {
            report << "fatal: function '" + function.first + "' is empty" << endl;
            break;
        }
    }
    return report.str();
}
