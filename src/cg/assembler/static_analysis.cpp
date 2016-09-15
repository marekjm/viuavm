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


static auto skip_till_next_line(const std::vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> decltype(i) {
    do {
        ++i;
    } while (i < tokens.size() and tokens.at(i) != "\n");
    return i;
}
static string resolve_register_name(const map<string, string>& named_registers, viua::cg::lex::Token token) {
    string name = token;
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
static void check_timeout_operand(Token token) {
    if (token == "\n") {
        throw viua::cg::lex::InvalidSyntax(token, "missing timeout operand");
    }
    static const regex timeout_regex{"^(?:0|[1-9]\\d*)ms$"};
    if (token != "infinity" and not regex_match(token.str(), timeout_regex)) {
        throw viua::cg::lex::InvalidSyntax(token, "invalid timeout operand");
    }
}
static void check_block_body(const vector<viua::cg::lex::Token>& body_tokens, set<string>& defined_registers, const map<string, vector<viua::cg::lex::Token>>& block_bodies, const bool debug) {
    map<string, string> named_registers;
    for (decltype(body_tokens.size()) i = 0; i < body_tokens.size(); ++i) {
        auto token = body_tokens.at(i);
        if (token == "\n") {
            continue;
        }
        if (token == ".name:") {
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
            check_block_body(block_bodies.at(body_tokens.at(i+1)), defined_registers, block_bodies, debug);
            i = skip_till_next_line(body_tokens, i);
            continue;
        }
        if (token == "move") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("move from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            defined_registers.erase(defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "vpop" or token == "vat" or token == "vlen") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), (token.str() + " from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "vinsert" or token == "vpush") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), (token.str() + " into empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), (token.str() + " from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            defined_registers.erase(defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "insert") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("insert into empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("insert key from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+3))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+3), ("insert from empty register: " + str::strencode(body_tokens.at(i+3))));
            }
            defined_registers.erase(defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+3))));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "remove") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("remove from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+3))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+3), ("remove key from empty register: " + str::strencode(body_tokens.at(i+3))));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "delete") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("delete of empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            defined_registers.erase(defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))));
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "throw") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("throw from empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            defined_registers.erase(defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))));
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "isnull") {
            // it makes little sense to statically check "isnull" instruction for accessing null registers as it gracefully handles
            // accessing them;
            // "isnull" instruction is used, for example, to implement static variables in functions (during initialisation), and
            // in closures
            // it is not viable to statically check for register emptiness in the context of "isnull" instruction
            // instead, statically check for the "non-emptiness" and thrown an error is we can determine that the register access
            // will always be valid
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) != defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("useless check, register will always be defined: " + str::strencode(body_tokens.at(i+2))));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "tmpri") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("move to tmp register from empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            defined_registers.erase(defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))));
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "tmpro") {
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "branch") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("branch depends on empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "echo" or token == "print" or token == "not") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), (token.str() + " of empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "pamv" or token == "param") {
            if (debug) {
                cout << str::enquote(token) << " from register " << str::enquote(str::strencode(body_tokens.at(i+2))) << endl;
            }
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("parameter " + string(token.str() == "pamv" ? "move" : "pass") + " from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            if (token == "pamv") {
                defined_registers.erase(defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))));
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "enclose" or token == "enclosecopy" or token == "enclosemove") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("closure of empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "copy") {
            string copy_from = body_tokens.at(i+2);
            if ((not str::isnum(copy_from)) and named_registers.count(copy_from)) {
                copy_from = named_registers.at(copy_from);
            }
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("copy from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "ptr") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("pointer from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "fcall") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), (token.str() + " from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "send") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("send from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("send target from empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            defined_registers.erase(defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "swap") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("swap with empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("swap with empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "itof" or token == "ftoi" or token == "stoi" or token == "stof") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("use of empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "vec") {
            ++i; // the "vec" token
            int starting_register = stoi(resolve_register_name(named_registers, body_tokens.at(i+1)));
            int registers_to_pack = stoi(resolve_register_name(named_registers, body_tokens.at(i+2)));
            if (registers_to_pack) {
                for (int j = starting_register; j < (starting_register+registers_to_pack); ++j) {
                    if (defined_registers.find(str::stringify(j, false)) == defined_registers.end()) {
                        throw viua::cg::lex::InvalidSyntax(token, ("packing empty register: " + str::stringify(j, false)));
                    }
                    defined_registers.erase(str::stringify(j, false));
                }
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "iadd" or token == "isub" or token == "imul" or token == "idiv" or
                   token == "ilt" or token == "ilte" or token == "igt" or token == "igte" or token == "ieq" or
                   token == "fadd" or token == "fsub" or token == "fmul" or token == "fdiv" or
                   token == "flt" or token == "flte" or token == "fgt" or token == "fgte" or token == "feq" or
                   token == "and" or token == "or") {
            ++i; // skip mnemonic token
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(token, ("use of empty register: " + body_tokens.at(i+1).str()));
            }
            if (body_tokens.at(i+2) != "\n" and defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(token, ("use of empty register: " + body_tokens.at(i+2).str()));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "join") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), (token.str() + " from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            check_timeout_operand(body_tokens.at(i+3));
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "receive") {
            string reg_original = body_tokens.at(i+1), reg = resolve_register_name(named_registers, body_tokens.at(i+1));
            defined_registers.insert(reg);
            check_timeout_operand(body_tokens.at(i+2));
            i = skip_till_next_line(body_tokens, i);
        } else {
            if (not ((token == "call" or token == "process") and body_tokens.at(i+1) == "0")) {
                string reg_original = body_tokens.at(i+1), reg = resolve_register_name(named_registers, body_tokens.at(i+1));
                defined_registers.insert(reg);
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
    set<string> defined_registers;
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
            check_block_body(body, defined_registers, block_bodies, debug);
            defined_registers.clear();
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
