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
    map<string, Token> defined_registers;
    map<string, Token> used_registers;
    map<string, Token> erased_registers;
    set<string> maybe_unused_registers;

    public:
    bool defined(const string& r) {
        return (defined_registers.count(r) == 1);
    }
    Token defined_where(string r) {
        return defined_registers.at(r);
    }
    void insert(string r, Token where) {
        if (r != "void") {
            defined_registers.emplace(r, where);
        }
    }
    void erase(const string& r, const Token& token) {
        erased_registers.emplace(r, token);
        defined_registers.erase(defined_registers.find(r));
    }
    bool erased(const string& r) {
        return (erased_registers.count(r) == 1);
    }
    Token erased_by(const string& r) {
        return erased_registers.at(r);
    }
    void use(string r, Token where) {
        used_registers.emplace(r, where);
    }
    bool used(string r) {
        return used_registers.count(r);
    }

    void unused(string r) {
        maybe_unused_registers.insert(r);
    }
    auto maybe_unused(string r) -> bool {
        return (maybe_unused_registers.count(r) != 0);
    }

    auto begin() -> decltype(defined_registers.begin()) {
        return defined_registers.begin();
    }
    auto end() -> decltype(defined_registers.end()) {
        return defined_registers.end();
    }
};


static auto skip_till_next_line(const std::vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> decltype(i) {
    do {
        ++i;
    } while (i < tokens.size() and tokens.at(i) != "\n");
    return i;
}
static auto is_named(const map<string, string>& named_registers, string name) -> bool {
    return (std::find_if(named_registers.begin(), named_registers.end(), [name](std::remove_reference<decltype(named_registers)>::type::value_type t) {
        return (t.second == name);
    }) != named_registers.end());
}
static auto get_name(const map<string, string>& named_registers, string name, Token context) -> string {
    auto it = std::find_if(named_registers.begin(), named_registers.end(), [name](std::remove_reference<decltype(named_registers)>::type::value_type t) {
        return (t.second == name);
    });
    if (it == named_registers.end()) {
        throw viua::cg::lex::InvalidSyntax(context, ("register is not named: " + name));
    }
    return it->first;
}
static string resolve_register_name(const map<string, string>& named_registers, viua::cg::lex::Token token, string name, const bool allow_direct_access = false) {
    if (name == "\n") {
        throw viua::cg::lex::InvalidSyntax(token, "expected operand, found newline");
    }
    if (name == "void") {
        return name;
    }
    if (name.at(0) == '@' or name.at(0) == '*' or name.at(0) == '%') {
        name = name.substr(1);
    }
    if (str::isnum(name, false)) {
        if ((not allow_direct_access) and is_named(named_registers, name) and not (token.original() == "iota" or ((token.original().at(0) == '%' or token.original().at(0) == '@' or token.original().at(0) == '*') and token.original().substr(1) == "iota") or token.original() == "\n")) {
            throw viua::cg::lex::InvalidSyntax(token, ("accessing named register using direct index: " + str::enquote(get_name(named_registers, name, token)) + " := " + name));
        }
        return name;
    }
    if (str::isnum(name, true)) {
        throw viua::cg::lex::InvalidSyntax(token, ("register indexes cannot be negative: " + name));
    }
    if (named_registers.count(name) == 0) {
        throw viua::cg::lex::InvalidSyntax(token, ("not a named register: " + str::enquote(str::strencode(name))));
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
    static const regex timeout_regex{"^(?:0|[1-9]\\d*)m?s$"};
    if (token != "infinity" and not regex_match(token.str(), timeout_regex)) {
        throw viua::cg::lex::InvalidSyntax(token, "invalid timeout operand");
    }
}

static auto strip_access_mode_sigil(string s) -> string {
    return ((s.at(0) == '%' or s.at(0) == '@' or s.at(0) == '*') ? s.substr(1) : s);
}
static void check_use_of_register_index(const vector<viua::cg::lex::Token>& tokens, long unsigned i, long unsigned by, string register_index, Registers& registers, map<string, string>& named_registers, const string& message_prefix, const bool allow_direct_access_to_target = true) {
    string resolved_register_name;
    try {
        resolved_register_name = resolve_register_name(named_registers, tokens.at(i), register_index, allow_direct_access_to_target);
    } catch (viua::cg::lex::InvalidSyntax& e) {
        e.add(tokens.at(by));
        throw e;
    }
    if (resolved_register_name == "void") {
        auto base_error = viua::cg::lex::InvalidSyntax(tokens.at(i), "use of void as input register:");
        base_error.add(tokens.at(by));
        throw base_error;
    }
    if (not registers.defined(resolved_register_name)) {
        string message = (message_prefix + ": " + str::strencode(strip_access_mode_sigil(register_index)));
        if (resolved_register_name != strip_access_mode_sigil(register_index)) {
            message += (" := " + resolved_register_name);
        }
        auto base_error = viua::cg::lex::InvalidSyntax(tokens.at(i), message);
        base_error.add(tokens.at(by));
        if (not registers.erased(resolved_register_name)) {
            throw base_error;
        }
        viua::cg::lex::TracedSyntaxError traced_error;
        traced_error.append(base_error);
        traced_error.append(viua::cg::lex::InvalidSyntax(registers.erased_by(resolved_register_name), "erased by:"));
        throw traced_error;
    }
    registers.use(resolved_register_name, tokens.at(i));
}
static void check_use_of_register(const vector<viua::cg::lex::Token>& tokens, long unsigned i, long unsigned by, Registers& registers, map<string, string>& named_registers, const string& message_prefix, const bool allow_direct_access_to_target = true) {
    check_use_of_register_index(tokens, i, by, tokens.at(i), registers, named_registers, message_prefix, allow_direct_access_to_target);
}
static void check_use_of_register(const vector<viua::cg::lex::Token>& tokens, long unsigned i, long unsigned by, Registers& registers, map<string, string>& named_registers) {
    check_use_of_register_index(tokens, i, by, tokens.at(i), registers, named_registers, "use of empty register");
}

static void check_defined_but_unused(Registers& registers) {
    for (auto it = registers.begin(); it != registers.end(); ++it) {
        const auto& each = *it;
        if (each.first == "0") {
            continue;
        }
        if ((not registers.used(each.first)) and (not registers.erased(each.first)) and (not registers.maybe_unused(each.first))) {
            throw viua::cg::lex::UnusedValue(each.second);
        }
    }
}

static auto in_block_offset(const vector<viua::cg::lex::Token>& body_tokens, std::remove_reference<decltype(body_tokens)>::type::size_type i, Registers& registers, map<string, string>& named_registers) -> decltype(i) {
    const auto& checked_token = body_tokens.at(i);

    if (checked_token.str().at(0) == '+') {
        auto n = stoul(checked_token.str().substr(1));
        while (n--) {
            i = skip_till_next_line(body_tokens, i);
            ++i;
            if (str::startswith(body_tokens.at(i), ".")) {
                ++n;
            }
            if (body_tokens.at(i) == ".name:") {
                if (named_registers.count(body_tokens.at(i+2))) {
                    throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("register name already taken: " + str::strencode(body_tokens.at(i+2))));
                }
                if (registers.defined(body_tokens.at(i+1))) {
                    throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("register defined before being named: " + str::strencode(body_tokens.at(i+1)) + " = " + str::strencode(body_tokens.at(i+2))));
                }
                named_registers[body_tokens.at(i+2)] = strip_access_mode_sigil(body_tokens.at(i+1));
            }
        }
        --i;
    } else if (checked_token.str().at(0) == '-') {
        // FIXME: no support for negative jumps
        // not safe - if SA only jumps forwards it *will* finish analysis
        // but jumping backwards can create endless loops
        i = skip_till_next_line(body_tokens, i);
    } else if (checked_token.str().at(0) == '.') {
        // FIXME: no support for absolute jumps
        // not certain - maybe support for absolute jumps should be removed since
        // they cannot be easily checked by the SA; neither in this check, nor in
        // any other one - they are just ignored
        i = skip_till_next_line(body_tokens, i);
    } else if (str::isnum(checked_token)) {
        // FIXME: no support for non-relative jumps
        i = skip_till_next_line(body_tokens, i);
    } else {
        auto saved_i = i;
        while (i < body_tokens.size()-1 and not (body_tokens.at(i) == ".mark:" and body_tokens.at(i+1) == checked_token.str())) {
            i = skip_till_next_line(body_tokens, i);
            ++i;
            if (i < body_tokens.size()-1 and body_tokens.at(i) == ".name:") {
                if (named_registers.count(body_tokens.at(i+2))) {
                    throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("register name already taken: " + str::strencode(body_tokens.at(i+2))));
                }
                if (registers.defined(body_tokens.at(i+1))) {
                    throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("register defined before being named: " + str::strencode(body_tokens.at(i+1)) + " = " + str::strencode(body_tokens.at(i+2))));
                }
                named_registers[body_tokens.at(i+2)] = strip_access_mode_sigil(body_tokens.at(i+1));
            }
        }
        if (i >= body_tokens.size()-1) {
            // if this 'if' block is entered, the jump has been to a label defined before the jump
            // SA does not support backward jumps so just skip the jump and
            // go to next instruction
            i = skip_till_next_line(body_tokens, saved_i);
        }
    }

    return i;
}

static void check_block_body(const vector<viua::cg::lex::Token>& body_tokens, decltype(body_tokens.size()), Registers&, const map<string, vector<viua::cg::lex::Token>>&, const bool);
static void check_block_body(const vector<viua::cg::lex::Token>&, Registers&, const map<string, vector<viua::cg::lex::Token>>&, const bool);

static void erase_register(Registers& registers, map<string, string>& named_registers, const Token& name, const Token& context) {
    /*
     * Even if normally an instruction would erase a register, when it is a pointer dereference that is given as the opernad the
     * "move an object" becomes a "copy an object pointed-to", and the pointer is left untouched.
     */
    if (name.str().at(0) != '*') {
        registers.erase(resolve_register_name(named_registers, name), context);
    }
}

// FIXME this function is duplicated
static auto get_token_index_of_operand(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i, int wanted_operand_index) -> decltype(i) {
    auto limit = tokens.size();
    while (i < limit and wanted_operand_index > 0) {
        //if (not (tokens.at(i) == "," or viua::cg::lex::is_reserved_keyword(tokens.at(i)))) {
        auto token = tokens.at(i);
        bool is_valid_operand = (str::isnum(token, false) or str::isid(token) or ((token.str().at(0) == '%' or token.str().at(0) == '@' or token.str().at(0) == '*') and (str::isnum(token.str().substr(1)) or str::isid(token.str().substr(1)))));
        bool is_valid_operand_area_token = (token == "," or token == "static" or token == "local" or token == "global");
        if (is_valid_operand_area_token) {
            ++i;
        } else if (is_valid_operand) {
            ++i;
            --wanted_operand_index;
        } else {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), "invalid token where operand index was expected");
        }
    }
    return i;
}

static void check_block_body(const vector<viua::cg::lex::Token>& body_tokens, decltype(body_tokens.size()) i, Registers& registers, map<string, string> named_registers, const map<string, vector<viua::cg::lex::Token>>& block_bodies, const bool debug) {
    using TokenIndex = std::remove_reference<decltype(body_tokens)>::type::size_type;

    for (; i < body_tokens.size(); ++i) {
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
            named_registers[body_tokens.at(i+2)] = strip_access_mode_sigil(body_tokens.at(i+1));
            if (debug) {
                cout << "  " << "register " << str::enquote(str::strencode(body_tokens.at(i+1))) << " is named " << str::enquote(str::strencode(body_tokens.at(i+2))) << endl;
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        }
        if (token == ".unused:") {
            registers.unused(strip_access_mode_sigil(body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        }
        if (token == "leave") {
            return;
        }
        if (token == "return") {
            check_defined_but_unused(registers);
            return;
        }
        if (token == "ress") {
            if (body_tokens.at(i+1) == "global" or body_tokens.at(i+1) == "static") {
                // FIXME SA cannot reliably verify functions that use global or static registers
                return;
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        }
        if (token == ".mark:" or token == ".link:" or token == "nop" or token == "tryframe" or token == "try" or token == "catch" or token == "frame" or
            token == "tailcall" or token == "halt" or
            token == "watchdog" or
            token == "link" or token == "import") {
            i = skip_till_next_line(body_tokens, i);
            continue;
        }

        if (token == "enter") {
            try {
                check_block_body(block_bodies.at(body_tokens.at(i+1)), 0, registers, block_bodies, debug);
            } catch (viua::cg::lex::InvalidSyntax& e) {
                viua::cg::lex::InvalidSyntax base_error {token, ("after entering block " + str::enquote(body_tokens.at(i+1).original()))};
                base_error.add(body_tokens.at(i+1));
                throw viua::cg::lex::TracedSyntaxError().append(e).append(base_error);
            } catch (viua::cg::lex::TracedSyntaxError& e) {
                viua::cg::lex::InvalidSyntax base_error {token, ("after entering block " + str::enquote(body_tokens.at(i+1).original()))};
                base_error.add(body_tokens.at(i+1));
                throw e.append(base_error);
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        }

        if (token == "jump") {
            i = in_block_offset(body_tokens, i+1, registers, named_registers)-1;
            continue;
        }

        if (token == "move") {
            TokenIndex target = i + 1;
            TokenIndex source = target + 2;

            check_use_of_register(body_tokens, source, i, registers, named_registers, "move from empty register");
            registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));
            erase_register(registers, named_registers, body_tokens.at(source), token);

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "vpop") {
            TokenIndex target = i + 1;
            TokenIndex source = target + 2;

            if (body_tokens.at(target) == "void") {
                // source is one token earlier since void has no register set
                --source;
            }

            check_use_of_register(body_tokens, source, i, registers, named_registers, (token.str() + " from empty register"));
            registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "vat" or token == "vlen") {
            TokenIndex target = i + 1;
            TokenIndex source = target + 2;

            check_use_of_register(body_tokens, source, i, registers, named_registers, (token.str() + " from empty register"));
            registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "vinsert" or token == "vpush") {
            TokenIndex target = i + 1;
            TokenIndex source = target + 2;

            check_use_of_register(body_tokens, target, i, registers, named_registers, (token.str() + " into empty register"));
            check_use_of_register(body_tokens, source, i, registers, named_registers, (token.str() + " from empty register"));
            erase_register(registers, named_registers, body_tokens.at(source), token);

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "insert") {
            TokenIndex source = get_token_index_of_operand(body_tokens, i, 3);
            TokenIndex key = get_token_index_of_operand(body_tokens, i, 2);
            TokenIndex target = get_token_index_of_operand(body_tokens, i, 1);

            check_use_of_register(body_tokens, target, i, registers, named_registers, "insert into empty register");
            check_use_of_register(body_tokens, key, i, registers, named_registers, "insert key from empty register");
            check_use_of_register(body_tokens, source, i, registers, named_registers, "insert from empty register");
            erase_register(registers, named_registers, body_tokens.at(source), token);

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "remove") {
            TokenIndex source = get_token_index_of_operand(body_tokens, i, 2);
            TokenIndex key = get_token_index_of_operand(body_tokens, i, 3);
            TokenIndex target = get_token_index_of_operand(body_tokens, i, 1);

            check_use_of_register(body_tokens, source, i, registers, named_registers, "remove from empty register");
            check_use_of_register(body_tokens, key, i, registers, named_registers, "remove key from empty register");
            registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(i+1));

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "delete") {
            TokenIndex target = get_token_index_of_operand(body_tokens, i, 1);

            check_use_of_register(body_tokens, target, i, registers, named_registers, "delete of empty register");
            erase_register(registers, named_registers, body_tokens.at(target), token);

            i = skip_till_next_line(body_tokens, i);
        } else if (token == "throw") {
            TokenIndex source = get_token_index_of_operand(body_tokens, i, 1);

            check_use_of_register(body_tokens, source, i, registers, named_registers, "throw from empty register");
            erase_register(registers, named_registers, body_tokens.at(source), token);

            i = skip_till_next_line(body_tokens, i);
        } else if (token == "isnull") {
            // it makes little sense to statically check "isnull" instruction for accessing null registers as it gracefully handles
            // accessing them;
            // "isnull" instruction is used, for example, to implement static variables in functions (during initialisation), and
            // in closures
            // it is not viable to statically check for register emptiness in the context of "isnull" instruction
            // instead, statically check for the "non-emptiness" and thrown an error is we can determine that the register access
            // will always be valid
            TokenIndex target = i + 1;
            TokenIndex source = target + 2;

            if (registers.defined(resolve_register_name(named_registers, body_tokens.at(source)))) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(target), ("useless check, register will always be defined: " + str::strencode(strip_access_mode_sigil(body_tokens.at(source)))));
            }
            registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "tmpri") {
            TokenIndex source = get_token_index_of_operand(body_tokens, i, 1);

            check_use_of_register(body_tokens, source, i, registers, named_registers, "move to tmp register from empty register");
            erase_register(registers, named_registers, body_tokens.at(source), token);

            i = skip_till_next_line(body_tokens, i);
        } else if (token == "tmpro") {
            TokenIndex target = get_token_index_of_operand(body_tokens, i, 1);

            registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));

            i = skip_till_next_line(body_tokens, i);
        } else if (token == "if") {
            ++i;
            TokenIndex source = i;
            check_use_of_register(body_tokens, source, i-1, registers, named_registers, "branch depends on empty register");

            string register_with_unused_value;

            try {
                auto copied_registers = registers;
                auto copied_named_registers = named_registers;
                check_block_body(body_tokens, in_block_offset(body_tokens, i+1, copied_registers, copied_named_registers), copied_registers, copied_named_registers, block_bodies, debug);
            } catch (viua::cg::lex::UnusedValue& e) {
                // do not fail yet, because the value may be used by false branch
                // save the error for later
                register_with_unused_value = e.what();
            } catch (const viua::cg::lex::InvalidSyntax& e) {
                throw viua::cg::lex::TracedSyntaxError().append(e).append(viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), "after taking true branch:"));
            } catch (viua::cg::lex::TracedSyntaxError& e) {
                throw e.append(viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), "after taking true branch:"));
            }
            try {
                auto copied_registers = registers;
                auto copied_named_registers = named_registers;
                check_block_body(body_tokens, in_block_offset(body_tokens, i+2, copied_registers, copied_named_registers), copied_registers, copied_named_registers, block_bodies, debug);
            } catch (viua::cg::lex::UnusedValue& e) {
                if (register_with_unused_value == e.what()) {
                    throw viua::cg::lex::TracedSyntaxError().append(e).append(viua::cg::lex::InvalidSyntax(body_tokens.at(i-1), "after taking either branch at:"));
                }
            } catch (const viua::cg::lex::InvalidSyntax& e) {
                throw viua::cg::lex::TracedSyntaxError().append(e).append(viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), "after taking false branch:"));
            } catch (viua::cg::lex::TracedSyntaxError& e) {
                throw e.append(viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), "after taking false branch:"));
            }
            // early return because we already checked both true, and false branches
            return;
        } else if (token == "echo" or token == "print") {
            TokenIndex source = get_token_index_of_operand(body_tokens, i, 1);

            check_use_of_register(body_tokens, source, i, registers, named_registers, (token.str() + " of empty register"), false);

            i = skip_till_next_line(body_tokens, i);
        } else if (token == "not") {
            TokenIndex target = i + 1;
            TokenIndex source = target + 2;

            check_use_of_register(body_tokens, source, i, registers, named_registers, (token.str() + " of empty register"));
            registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));

            i = skip_till_next_line(body_tokens, i);
        } else if (token == "pamv" or token == "param") {
            TokenIndex source = get_token_index_of_operand(body_tokens, i, 2);

            if (debug) {
                cout << str::enquote(token) << " from register " << str::enquote(str::strencode(body_tokens.at(source))) << endl;
            }
            check_use_of_register(body_tokens, source, i, registers, named_registers, ("parameter " + string(token.str() == "pamv" ? "move" : "pass") + " from empty register"));
            if (token == "pamv") {
                erase_register(registers, named_registers, body_tokens.at(source), token);
            }

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "capture" or token == "capturecopy" or token == "capturemove") {
            TokenIndex source = get_token_index_of_operand(body_tokens, i, 3);

            // FIXME check if target is not empty
            check_use_of_register(body_tokens, source, i, registers, named_registers, "closure of empty register");

            i = skip_till_next_line(body_tokens, i);
        } else if (token == "copy" or token == "ptr") {
            TokenIndex target = i + 1;
            TokenIndex source = target + 2;

            string opcode_name = token;
            check_use_of_register(body_tokens, source, i, registers, named_registers, ((opcode_name == "ptr" ? "pointer" : opcode_name) + " from empty register"));
            registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "fcall") {
            TokenIndex source = get_token_index_of_operand(body_tokens, i, 2);
            TokenIndex target = get_token_index_of_operand(body_tokens, i, 1);

            string opcode_name = token;
            check_use_of_register(body_tokens, source, i, registers, named_registers, ((opcode_name == "ptr" ? "pointer" : opcode_name) + " from empty register"));
            registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "send") {
            TokenIndex source = get_token_index_of_operand(body_tokens, i, 2);
            TokenIndex target = get_token_index_of_operand(body_tokens, i, 1);

            check_use_of_register(body_tokens, source, i, registers, named_registers, "send from empty register");
            check_use_of_register(body_tokens, target, i, registers, named_registers, "send target from empty register");
            erase_register(registers, named_registers, body_tokens.at(source), token);

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "swap") {
            TokenIndex target = i + 1;
            TokenIndex source = target + 2;

            check_use_of_register(body_tokens, target, i, registers, named_registers, "swap with empty register");
            check_use_of_register(body_tokens, source, i, registers, named_registers, "swap with empty register");

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "itof" or token == "ftoi" or token == "stoi" or token == "stof") {
            TokenIndex target = i + 1;
            TokenIndex source = target + 2;

            check_use_of_register(body_tokens, source, i, registers, named_registers);
            registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "vec") {
            ++i; // the "vec" token

            TokenIndex target = i;
            TokenIndex pack_range_start = target + 2;
            TokenIndex pack_range_count = pack_range_start + 2;

            int starting_register = stoi(resolve_register_name(named_registers, body_tokens.at(pack_range_start)));

            if (body_tokens.at(pack_range_count).str().at(0) != '%') {
                auto error = viua::cg::lex::InvalidSyntax(body_tokens.at(pack_range_count), "expected register index operand");
                error.add(body_tokens.at(i-1));
                throw error;
            }
            int registers_to_pack = stoi(body_tokens.at(pack_range_count).str().substr(1));
            if (registers_to_pack) {
                for (int j = starting_register; j < (starting_register+registers_to_pack); ++j) {
                    check_use_of_register_index(body_tokens, i-1, i-1, str::stringify(j, false), registers, named_registers, "packing empty register");
                    registers.erase(str::stringify(j, false), token);
                }
            }
            registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "and" or token == "or") {
            ++i; // skip mnemonic token

            TokenIndex rhs = get_token_index_of_operand(body_tokens, i, 2);
            TokenIndex lhs = get_token_index_of_operand(body_tokens, i, 1);

            check_use_of_register(body_tokens, lhs, i, registers, named_registers);
            check_use_of_register(body_tokens, rhs, i, registers, named_registers);
            registers.insert(resolve_register_name(named_registers, body_tokens.at(i)), body_tokens.at(i));

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "add" or token == "sub" or token == "mul" or token == "div" or
                   token == "lt" or token == "lte" or token == "gt" or token == "gte" or token == "eq") {
            ++i; // skip mnemonic token
            ++i; // skip result type token

            TokenIndex target = i;
            TokenIndex lhs = target + 2;
            TokenIndex rhs = lhs + 2;

            check_use_of_register(body_tokens, lhs, i, registers, named_registers);
            check_use_of_register(body_tokens, rhs, i, registers, named_registers);
            registers.insert(resolve_register_name(named_registers, body_tokens.at(i)), body_tokens.at(i));

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "join") {
            TokenIndex timeout = get_token_index_of_operand(body_tokens, i, 3);
            TokenIndex source = get_token_index_of_operand(body_tokens, i, 2);
            TokenIndex target = get_token_index_of_operand(body_tokens, i, 1);

            check_timeout_operand(body_tokens.at(timeout));
            check_use_of_register(body_tokens, source, i, registers, named_registers, "join from empty register");
            if (body_tokens.at(target) != "void") {
                registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));
            }

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "receive") {
            TokenIndex timeout = get_token_index_of_operand(body_tokens, i, 2);
            TokenIndex target = get_token_index_of_operand(body_tokens, i, 1);

            check_timeout_operand(body_tokens.at(timeout));
            if (body_tokens.at(target) != "void") {
                registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));
            }
            check_timeout_operand(body_tokens.at(timeout));

            i = skip_till_next_line(body_tokens, i);
        } else if (token == "iinc" or token == "idec") {
            // skip mnemonic
            ++i;
            check_use_of_register(body_tokens, i, i-1, registers, named_registers, "use of empty register");

            i = skip_till_next_line(body_tokens, i);
        } else if (token == "register") {
            TokenIndex target = get_token_index_of_operand(body_tokens, i, 1);

            check_use_of_register(body_tokens, target, i, registers, named_registers, "registering class from empty register");
            erase_register(registers, named_registers, body_tokens.at(target), token);

            i = skip_till_next_line(body_tokens, i);
        } else if (token == "msg" or token == "call" or token == "process") {
            TokenIndex target = get_token_index_of_operand(body_tokens, i, 1);

            if (body_tokens.at(target) != "void") {
                registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "arg") {
            TokenIndex target = get_token_index_of_operand(body_tokens, i, 1);

            if (not (body_tokens.at(target) == "void")) {
                registers.insert(resolve_register_name(named_registers, body_tokens.at(target)), body_tokens.at(target));
            }

            i = skip_till_next_line(body_tokens, i);
            continue;
        } else {
            TokenIndex target = get_token_index_of_operand(body_tokens, i, 1);

            string reg_original = body_tokens.at(target), reg = resolve_register_name(named_registers, body_tokens.at(target));
            registers.insert(reg, body_tokens.at(target));
            if (debug) {
                cout << "  " << str::enquote(token) << " defined register " << str::enquote(str::strencode(reg_original));
                if (reg != reg_original) {
                    cout << " = " << str::enquote(str::strencode(reg));
                }
                cout << endl;
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        }
    }
    check_defined_but_unused(registers);
}
static void check_block_body(const vector<viua::cg::lex::Token>& body_tokens, decltype(body_tokens.size()) i, Registers& registers, const map<string, vector<viua::cg::lex::Token>>& block_bodies, const bool debug) {
    map<string, string> named_registers;
    check_block_body(body_tokens, i, registers, named_registers, block_bodies, debug);
}
static void check_block_body(const vector<viua::cg::lex::Token>& body_tokens, Registers& registers, const map<string, vector<viua::cg::lex::Token>>& block_bodies, const bool debug) {
    check_block_body(body_tokens, 0, registers, block_bodies, debug);
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
            try {
                check_block_body(body, registers, block_bodies, debug);
            } catch (const viua::cg::lex::InvalidSyntax& e) {
                throw viua::cg::lex::TracedSyntaxError().append(e).append(viua::cg::lex::InvalidSyntax(tokens.at(i-body.size()-2), ("in function " + opened_function)));
            } catch (viua::cg::lex::TracedSyntaxError& e) {
                throw e.append(viua::cg::lex::InvalidSyntax(tokens.at(i-body.size()-2), ("in function " + opened_function)));
            }
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
