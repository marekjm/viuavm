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

string assembler::verify::functionCallArities(const string& filename, const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines, bool warning) {
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
        if (str::isnum(function_name)) {
            function_name = str::chunk(str::lstrip(line.substr(function_name.size())));
        }

        if (not assembler::utils::isValidFunctionName(function_name)) {
            report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: '" << function_name << "' is not a valid function name";
            break;
        }

        int arity = assembler::utils::getFunctionArity(function_name);

        if (arity == -1) {
            if (warning) {
                // arity of the function was not given - skip the check since there is no indication of the correct number of parameters but
                // print a warning
                cout << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": warning: call to function with undefined arity ";
                if (frame_parameters_count >= 0) {
                    cout << "as " << function_name << '/' << frame_parameters_count;
                } else {
                    cout << ": " << function_name;
                }
                cout << endl;
            }
            continue;
        }
        if (frame_parameters_count == -1) {
            // frame paramters count could not be statically determined, deffer the check until runtime
            continue;
        }

        if (arity != frame_parameters_count) {
            report << filename << ':' << expanded_lines_to_source_lines.at(i)+1 << ": error: invalid number of parameters in call to function " << function_name << ": expected " << arity << " got " << frame_parameters_count;
            break;
        }
    }
    return report.str();
}

string assembler::verify::functionNames(const string& filename, const std::vector<std::string>& lines, const bool warning, const bool error) {
    ostringstream report("");
    string line;
    string function;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (str::startswith(line, ".function:")) {
            function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
        } else {
            continue;
        }

        if (not assembler::utils::isValidFunctionName(function)) {
            report << filename << ':' << i+1 << ": error: invalid function name: " << function;
            break;
        }

        // FIXME: main with undefined arity is allowed for now
        // it will be an error not to give arity to main in future release
        if (assembler::utils::getFunctionArity(function) == -1 and function != "main") {
            if (warning) {
                cout << filename << ':' << i+1 << ": warning: function with undefined arity: " << function << endl;
            } else if (error) {
                report << filename << ':' << i+1 << ": error: function with undefined arity: " << function;
                break;
            } else {
                // explicitly do nothing if neither warning nor error report was requested
            }
        }
    }

    return report.str();
}

string assembler::verify::functionsEndWithReturn(const string& filename, const std::vector<std::string>& lines, const bool warning, const bool error) {
    ostringstream report("");
    string line;
    string function;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (str::startswith(line, ".function:")) {
            function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            continue;
        } else if (str::startswithchunk(line, ".end") and function.size()) {
            // .end may have been reached while not in a function because blocks also end with .end
            // so we also make sure that we were inside a function
        } else {
            continue;
        }

        if (i and (not str::startswithchunk(str::lstrip(lines[i-1]), "return"))) {
            if (warning) {
                cout << filename << ':' << i+1 << ": warning: missing 'return' at the end of function " << function << endl;
            } else if (error) {
                report << filename << ':' << i+1 << ": error: missing 'return' at the end of function " << function;
                break;
            } else {
                // explicitly do nothing if neither warning nor error report was requested
            }
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
        if (str::startswith(line, ".function:")) {
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
        if (registerset_name == "global" and as_lib and function != "main") {
            report << filename << ':' << (expanded_lines_to_source_lines.at(i)+1) << ": error: global registers used in library function " << function;
            break;
        }
    }
    return report.str();
}

string bodiesAreNonempty(const string& filename, const vector<string>& lines, const string& type) {
    ostringstream report("");
    string line;
    string name;

    string prefix = ("." + type + ":");

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (str::startswith(line, prefix)) {
            // prefix ('.function:' or '.block:') is interesting from the point of our analysis
            name = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            continue;
        } else if (str::startswithchunk(line, ".end")) {
            // '.end' is also interesting because we want to see if it's immediately preceded by
            // the interesting prefix
        } else {
            continue;
        }

        // this if is reached only of '.end' was matched - so we just check if it is preceded by
        // the interesting prefix, and
        // if it is - report an error
        if (i and str::startswithchunk(str::lstrip(lines[i-1]), prefix)) {
            report << filename << ':' << i << ": error: " << type << " with empty body: " << name;
            break;
        }
    }

    return report.str();
}
string assembler::verify::functionBodiesAreNonempty(const std::string& filename, const vector<string>& lines) {
    return bodiesAreNonempty(filename, lines, "function");
}

string assembler::verify::blockBodiesAreNonempty(const string& filename, const std::vector<std::string>& lines) {
    return bodiesAreNonempty(filename, lines, "block");
}

string assembler::verify::mainFunctionDoesNotEndWithHalt(map<string, vector<string> >& functions) {
    ostringstream report("");
    string line;
    if (functions.count("main") == 0) {
        report << "error: cannot verify undefined 'main' function" << endl;
        return report.str();
    }
    vector<string> flines = functions.at("main");
    if (flines.size() == 0) {
        report << "error: cannot verify empty 'main' function" << endl;
        return report.str();
    }
    if (str::chunk(str::lstrip(flines.back())) == "halt") {
        report << "error: using 'halt' instead of 'return' as last instruction in main function leads to memory leaks" << endl;
    }
    return report.str();
}

string assembler::verify::directives(const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (line.size() == 0 or line[0] != '.') {
            continue;
        }

        string token = str::chunk(line);
        if (not (token == ".function:" or token == ".signature:" or token == ".bsignature:" or token == ".block:" or token == ".end" or token == ".name:" or token == ".mark:" or token == ".main:" or token == ".type:" or token == ".class:")) {
            report << "fatal: unrecognised assembler directive on line ";
            report << (expanded_lines_to_source_lines.at(i)+1);
            report << ": `" << token << '`';
            break;
        }
    }
    return report.str();
}
string assembler::verify::instructions(const vector<string>& lines, const map<long unsigned, long unsigned>& expanded_lines_to_source_lines) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (line.size() == 0 or line[0] == '.' or line[0] == ';') {
            continue;
        }

        string token = str::chunk(line);
        if (OP_SIZES.count(token) == 0) {
            report << "fatal: unrecognised instruction on line ";
            report << (expanded_lines_to_source_lines.at(i)+1);
            report << ": `" << token << '`';
            break;
        }
    }
    return report.str();
}
