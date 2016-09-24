/*
 *  Copyright (C) 2016 Marek Marecki
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


using Token = viua::cg::lex::Token;

class Registers {
    map<string, long unsigned> defined_registers;
    map<string, long unsigned> used_registers;

    public:
    bool defined(const string& r) {
        //cout << "defined(r = " << r << ")\n";
        return (defined_registers.count(r) == 1);
    }
    void insert(string r, long unsigned where) {
        //cout << "insert(r = " << r << ", where = " << where << ")\n";
        defined_registers.emplace(r, where);
    }
    void erase(const string& r) {
        //cout << "erase(r = " << r << ")\n";
        defined_registers.erase(defined_registers.find(r));
    }
    void use(string r, long unsigned where) {
        used_registers.emplace(r, where);
    }
};


static auto skip_till_next_line(const std::vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> decltype(i) {
    do {
        ++i;
    } while (i < tokens.size() and tokens.at(i) != "\n");
    return i;
}
static string resolve_register_name(const map<string, string>& named_registers, viua::cg::lex::Token token, string name) {
    if (name.at(0) == '@') {
        name = name.substr(1);
    }
    if (str::isnum(name, false)) {
        return name;
    }
    if (str::isnum(name, true)) {
        throw viua::cg::lex::InvalidSyntax(token, ("register indexes cannot be negative: " + name));
    }
    if (named_registers.count(name) == 0) {
        throw viua::cg::lex::InvalidSyntax(token, ("not a named register: " + str::strencode(name)));
    }
    return named_registers.at(name);
}
static string resolve_register_name(const map<string, string>& named_registers, viua::cg::lex::Token token) {
    return resolve_register_name(named_registers, token, token.str());
}
static void check_timeout_operand(Token token) {
    if (token == "\n") {
        throw viua::cg::lex::InvalidSyntax(token, "missing timeout operand");
    }
    static const regex timeout_regex{"^(?:0|[1-9]\\d*)ms$"};
    if (token != "infinity" and not regex_match(token.str(), timeout_regex)) {
        throw viua::cg::lex::InvalidSyntax(token, "invalid timeout operand");
    }
}
static void check_use_of_register_index(const vector<viua::cg::lex::Token>& tokens, long unsigned i, string register_index, Registers& registers, map<string, string>& named_registers, const string& message_prefix) {
    string resolved_register_name = resolve_register_name(named_registers, tokens.at(i), register_index);
    if (not registers.defined(resolved_register_name)) {
        string message = (message_prefix + ": " + str::strencode(register_index));
        if (resolved_register_name != register_index) {
            message += (" := " + resolved_register_name);
        }
        throw viua::cg::lex::InvalidSyntax(tokens.at(i), message);
    }
}
static void check_use_of_register(const vector<viua::cg::lex::Token>& tokens, long unsigned i, Registers& registers, map<string, string>& named_registers, const string& message_prefix) {
    check_use_of_register_index(tokens, i, tokens.at(i), registers, named_registers, message_prefix);
}
static void check_use_of_register(const vector<viua::cg::lex::Token>& tokens, long unsigned i, Registers& registers, map<string, string>& named_registers) {
    check_use_of_register(tokens, i, registers, named_registers, "use of empty register");
}

static void check_block_body(const vector<viua::cg::lex::Token>& body_tokens, Registers& registers, const map<string, vector<viua::cg::lex::Token>>& block_bodies, const bool debug) {
    map<string, string> named_registers;
    for (decltype(body_tokens.size()) i = 0; i < body_tokens.size(); ++i) {
        auto token = body_tokens.at(i);
        if (token == "\n") {
            continue;
        }
        if (token == ".name:") {
            if (named_registers.count(body_tokens.at(i+2))) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("register name already taken: " + str::strencode(body_tokens.at(i+2))));
            }
            if (registers.defined(body_tokens.at(i+1))) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("register defined before being named: " + str::strencode(body_tokens.at(i+1)) + " = " + str::strencode(body_tokens.at(i+2))));
            }
            named_registers[body_tokens.at(i+2)] = body_tokens.at(i+1);
            if (debug) {
                cout << "  " << "register " << str::enquote(str::strencode(body_tokens.at(i+1))) << " is named " << str::enquote(str::strencode(body_tokens.at(i+2))) << endl;
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        }
        if (token == "leave" or token == "return") {
            return;
        }
        if (token == ".mark:" or token == "nop" or token == "tryframe" or token == "try" or token == "catch" or token == "frame" or
            token == "tailcall" or token == "halt" or
            token == "watchdog" or token == "jump" or
            token == "link" or token == "import" or token == "ress") {
            i = skip_till_next_line(body_tokens, i);
            continue;
        }
        if (token == "enter") {
            check_block_body(block_bodies.at(body_tokens.at(i+1)), registers, block_bodies, debug);
            i = skip_till_next_line(body_tokens, i);
            continue;
        }
        if (token == "move") {
            check_use_of_register(body_tokens, i+2, registers, named_registers, "move from empty register");
            registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)), i+1);
            registers.erase(resolve_register_name(named_registers, body_tokens.at(i+2)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "vpop" or token == "vat" or token == "vlen") {
            check_use_of_register(body_tokens, i+2, registers, named_registers, (token.str() + " from empty register"));
            registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)), i+1);
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "vinsert" or token == "vpush") {
            check_use_of_register(body_tokens, i+1, registers, named_registers, (token.str() + " into empty register"));
            check_use_of_register(body_tokens, i+2, registers, named_registers, (token.str() + " from empty register"));
            registers.erase(resolve_register_name(named_registers, body_tokens.at(i+2)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "insert") {
            check_use_of_register(body_tokens, i+1, registers, named_registers, "insert into empty register");
            check_use_of_register(body_tokens, i+2, registers, named_registers, "insert key from empty register");
            check_use_of_register(body_tokens, i+3, registers, named_registers, "insert from empty register");
            registers.erase(resolve_register_name(named_registers, body_tokens.at(i+3)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "remove") {
            check_use_of_register(body_tokens, i+2, registers, named_registers, "remove from empty register");
            check_use_of_register(body_tokens, i+3, registers, named_registers, "remove key from empty register");
            registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)), i+1);
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "delete") {
            check_use_of_register(body_tokens, i+1, registers, named_registers, "delete of empty register");
            registers.erase(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "throw") {
            check_use_of_register(body_tokens, i+1, registers, named_registers, "throw from empty register");
            registers.erase(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "isnull") {
            // it makes little sense to statically check "isnull" instruction for accessing null registers as it gracefully handles
            // accessing them;
            // "isnull" instruction is used, for example, to implement static variables in functions (during initialisation), and
            // in closures
            // it is not viable to statically check for register emptiness in the context of "isnull" instruction
            // instead, statically check for the "non-emptiness" and thrown an error is we can determine that the register access
            // will always be valid
            if (registers.defined(resolve_register_name(named_registers, body_tokens.at(i+2)))) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("useless check, register will always be defined: " + str::strencode(body_tokens.at(i+2))));
            }
            registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)), i+1);
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "tmpri") {
            check_use_of_register(body_tokens, i+1, registers, named_registers, "move to tmp register from empty register");
            registers.erase(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "tmpro") {
            registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)), i+1);
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "branch") {
            check_use_of_register(body_tokens, i+1, registers, named_registers, "branch depends on empty register");
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "echo" or token == "print" or token == "not") {
            check_use_of_register(body_tokens, i+1, registers, named_registers, (token.str() + " of empty register"));
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "pamv" or token == "param") {
            if (debug) {
                cout << str::enquote(token) << " from register " << str::enquote(str::strencode(body_tokens.at(i+2))) << endl;
            }
            check_use_of_register(body_tokens, i+2, registers, named_registers, ("parameter " + string(token.str() == "pamv" ? "move" : "pass") + " from empty register"));
            if (token == "pamv") {
                registers.erase(resolve_register_name(named_registers, body_tokens.at(i+2)));
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "enclose" or token == "enclosecopy" or token == "enclosemove") {
            check_use_of_register(body_tokens, i+2, registers, named_registers, "closure of empty register");
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "copy" or token == "ptr" or token == "fcall") {
            string opcode_name = token;
            check_use_of_register(body_tokens, i+2, registers, named_registers, ((opcode_name == "ptr" ? "pointer" : opcode_name) + " from empty register"));
            registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)), i+1);
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "send") {
            check_use_of_register(body_tokens, i+2, registers, named_registers, "send from empty register");
            check_use_of_register(body_tokens, i+1, registers, named_registers, "send target from empty register");
            registers.erase(resolve_register_name(named_registers, body_tokens.at(i+2)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "swap") {
            check_use_of_register(body_tokens, i+1, registers, named_registers, "swap with empty register");
            check_use_of_register(body_tokens, i+2, registers, named_registers, "swap with empty register");
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "itof" or token == "ftoi" or token == "stoi" or token == "stof") {
            check_use_of_register(body_tokens, i+2, registers, named_registers);
            registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)), i+1);
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "vec") {
            ++i; // the "vec" token
            int starting_register = stoi(resolve_register_name(named_registers, body_tokens.at(i+1)));
            int registers_to_pack = stoi(resolve_register_name(named_registers, body_tokens.at(i+2)));
            if (registers_to_pack) {
                for (int j = starting_register; j < (starting_register+registers_to_pack); ++j) {
                    check_use_of_register_index(body_tokens, i-1, str::stringify(j, false), registers, named_registers, "packing empty register");
                    registers.erase(str::stringify(j, false));
                }
            }
            registers.insert(resolve_register_name(named_registers, body_tokens.at(i)), i);
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "iadd" or token == "isub" or token == "imul" or token == "idiv" or
                   token == "ilt" or token == "ilte" or token == "igt" or token == "igte" or token == "ieq" or
                   token == "fadd" or token == "fsub" or token == "fmul" or token == "fdiv" or
                   token == "flt" or token == "flte" or token == "fgt" or token == "fgte" or token == "feq" or
                   token == "and" or token == "or") {
            ++i; // skip mnemonic token
            check_use_of_register(body_tokens, i+1, registers, named_registers);
            check_use_of_register(body_tokens, i+2, registers, named_registers);
            registers.insert(resolve_register_name(named_registers, body_tokens.at(i)), i);
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "join") {
            check_use_of_register(body_tokens, i+2, registers, named_registers, "join from empty register");
            check_timeout_operand(body_tokens.at(i+3));
            registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)), i+1);
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "receive") {
            string reg_original = body_tokens.at(i+1), reg = resolve_register_name(named_registers, body_tokens.at(i+1));
            registers.insert(reg, i);
            check_timeout_operand(body_tokens.at(i+2));
            i = skip_till_next_line(body_tokens, i);
        } else {
            if (not ((token == "call" or token == "process") and body_tokens.at(i+1) == "0")) {
                string reg_original = body_tokens.at(i+1), reg = resolve_register_name(named_registers, body_tokens.at(i+1));
                registers.insert(reg, 1);
                if (debug) {
                    cout << "  " << str::enquote(token) << " defined register " << str::enquote(str::strencode(reg_original));
                    if (reg != reg_original) {
                        cout << " = " << str::enquote(str::strencode(reg));
                    }
                    cout << endl;
                }
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        }
    }
}
void assembler::verify::manipulationOfDefinedRegisters(const std::vector<viua::cg::lex::Token>& tokens, const map<string, vector<viua::cg::lex::Token>>& block_bodies, const bool debug) {
    string opened_function;

    vector<viua::cg::lex::Token> body;
    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        auto token = tokens.at(i);
        if (token == ".function:") {
            opened_function = tokens.at(i+1);
            if (debug) {
                cout << "analysing '" << opened_function << "'\n";
            }
            i = skip_till_next_line(tokens, i);
            continue;
        }
        if (token == ".end") {
            if (debug) {
                cout << "running analysis of '" << opened_function << "' (" << body.size() << " tokens)\n";
            }
            Registers registers;
            check_block_body(body, registers, block_bodies, debug);
            body.clear();
            opened_function = "";
            i = skip_till_next_line(tokens, i);
            continue;
        }
        if (opened_function == "") {
            continue;
        }
        body.push_back(token);
    }
}
