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

#include <iostream>
#include <map>
#include <set>
#include <viua/assembler/frontend/static_analyser.h>
#include <viua/bytecode/operand_types.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/support/string.h>
using namespace std;
using namespace viua::assembler::frontend::parser;


using viua::cg::lex::Token;
using viua::cg::lex::InvalidSyntax;
using viua::cg::lex::TracedSyntaxError;


static auto invalid_syntax[[gnu::unused]](const vector<Token>& tokens, const string message)
    -> InvalidSyntax {
    auto invalid_syntax_error = InvalidSyntax(tokens.at(0), message);
    for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{1}; i < tokens.size(); ++i) {
        invalid_syntax_error.add(tokens.at(i));
    }
    return invalid_syntax_error;
}


struct Register {
    viua::internals::types::register_index index{0};
    viua::internals::RegisterSets register_set = viua::internals::RegisterSets::LOCAL;
    viua::internals::ValueTypes value_type = viua::internals::ValueTypes::UNDEFINED;

    auto operator<(const Register& that) const -> bool {
        if (register_set < that.register_set) {
            return true;
        }
        if (index < that.index) {
            return true;
        }
        return false;
    }
    auto operator==(const Register& that) const -> bool {
        return (register_set == that.register_set) and (index == that.index);
    }

    Register() = default;
    Register(const viua::assembler::frontend::parser::RegisterIndex& ri)
        : index(ri.index), register_set(ri.rss) {}
};

class RegisterUsageProfile {
    /*
     * Maps a register to the token that "defined" the register.
     * Consider:
     *
     *      text %1 local "Hello World!"    ; register 1 is defined, and
     *                                      ; the defining token is '%1'
     */
    map<Register, pair<Token, viua::internals::ValueTypes>> defined_registers;

    /*
     * Maps a register to the token marking the last place a register
     * has been used.
     * Consider:
     *
     *      text %1 local "Hello World!"    ; register 1 is defined, but unused
     *      print %1 local                  ; register 1 becomes used
     */
    map<Register, Token> used_registers;

    /*
     * Maps a register to the location where it was erased.
     * Note that not every register must be erased, and
     * when a register becomes defined it is no longer erased.
     * Consider:
     *
     *      text %1 local "Hello World!"    ; register 1 is defined
     *      delete %1 local                 ; register 1 is erased
     *      istore %1 local 42              ; register 1 is defined again
     */
    map<Register, Token> erased_registers;

    /*
     * The set of registers marked as "maybe unused".
     * If a register is unused at the end of a block, but has an entry in
     * this set an error should not be thrown for this register (but a warning, or
     * a different type of diagnostic message can be emitted).
     */
    set<Register> maybe_unused_registers;

  public:
    map<string, viua::internals::types::register_index> name_to_index;
    map<viua::internals::types::register_index, string> index_to_name;

    auto defined(const Register r) const -> bool { return defined_registers.count(r); }
    auto defined_where(const Register r) const -> Token { return defined_registers.at(r).first; }

    auto define(const Register r, const Token t) -> void {
        defined_registers.insert_or_assign(r, pair(t, r.value_type));
    }

    auto use(const Register r, const Token t) -> void { used_registers[r] = t; }
    auto used(const Register r) const -> bool { return used_registers.count(r); }

    auto at(const Register r) const -> const decltype(defined_registers)::mapped_type {
        return defined_registers.at(r);
    }

    auto begin() const -> decltype(defined_registers.begin()) { return defined_registers.begin(); }
    auto end() const -> decltype(defined_registers.end()) { return defined_registers.end(); }
};


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

template<typename K, typename V> static auto keys_of(const map<K, V>& m) -> vector<K> {
    vector<K> keys;

    for (const auto& each : m) {
        keys.push_back(each.first);
    }

    return keys;
}
static auto check_use_of_register(RegisterUsageProfile& rup,
                                  viua::assembler::frontend::parser::RegisterIndex r) {
    if (not r.resolved) {
        auto error = InvalidSyntax(r.tokens.at(0), "unresolved name");
        if (auto suggestion =
                str::levenshtein_best(r.tokens.at(0).str().substr(1), keys_of(rup.name_to_index), 4);
            suggestion.first) {
            error.aside("did you mean '" + suggestion.second + "' (name of " +
                        to_string(rup.name_to_index.at(suggestion.second)) + ")?");
        }
        throw error;
    }
    if (not rup.defined(Register(r))) {
        ostringstream msg;
        msg << "use of empty register " << str::enquote(to_string(r.index));
        if (rup.index_to_name.count(r.index)) {
            msg << " (named " << str::enquote(rup.index_to_name.at(r.index)) << ')';
        } else {
            msg << " (not named)";
        }
        throw InvalidSyntax(r.tokens.at(0), msg.str());
    }
    rup.use(Register(r), r.tokens.at(0));
}

auto value_type_names = map<viua::internals::ValueTypes, string>{
    {
        viua::internals::ValueTypes::UNDEFINED, "undefined"s,
    },
    {
        viua::internals::ValueTypes::VOID, "void"s,
    },
    {
        viua::internals::ValueTypes::INTEGER, "integer"s,
    },
    {
        viua::internals::ValueTypes::BOOLEAN, "boolean"s,
    },
    {
        viua::internals::ValueTypes::TEXT, "text"s,
    },
};
static auto to_string(const viua::internals::ValueTypes value_type_id) -> string {
    return value_type_names.at(value_type_id);
}

template<viua::internals::ValueTypes expected_type>
static auto check_type_of_register(const RegisterUsageProfile& register_usage_profile,
                                   const RegisterIndex& register_index) -> void {
    if (auto actual_type = register_usage_profile.at(Register(register_index)).second;
        actual_type != expected_type) {
        throw TracedSyntaxError{}
            .append(InvalidSyntax(register_index.tokens.at(0), "invalid type of value contained in register")
                        .note("expected " + to_string(expected_type) + ", got " + to_string(actual_type)))
            .append(InvalidSyntax(register_usage_profile.defined_where(Register(register_index)), "")
                        .note("defined here"));
    }
}

auto viua::assembler::frontend::static_analyser::check_register_usage(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource&, const InstructionsBlock& ib) -> void {
        RegisterUsageProfile register_usage_profile;

        for (const auto& line : ib.body) {
            auto directive = dynamic_cast<viua::assembler::frontend::parser::Directive*>(line.get());
            if (not directive) {
                continue;
            }

            if (directive->directive != ".name:") {
                continue;
            }

            // FIXME create strip_access_type_marker() function to implement this if
            auto idx = directive->operands.at(0);
            if (idx.at(0) == '%' or idx.at(0) == '*' or idx.at(0) == '@') {
                idx = idx.substr(1);
            }
            auto index = static_cast<viua::internals::types::register_index>(stoul(idx));
            auto name = directive->operands.at(1);

            register_usage_profile.name_to_index[name] = index;
            register_usage_profile.index_to_name[index] = name;
        }

        for (const auto& line : ib.body) {
            auto instruction = dynamic_cast<viua::assembler::frontend::parser::Instruction*>(line.get());
            if (not instruction) {
                continue;
            }

            using viua::assembler::frontend::parser::RegisterIndex;
            auto opcode = instruction->opcode;
            if (opcode == ISTORE) {
                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto val = Register{};
                val.index = operand->index;
                val.register_set = operand->rss;
                val.value_type = viua::internals::ValueTypes::INTEGER;
                register_usage_profile.define(val, operand->tokens.at(0));
            } else if (opcode == ADD or opcode == SUB or opcode == MUL or opcode == DIV or opcode == LT or
                       opcode == LTE or opcode == GT or opcode == GTE or opcode == EQ) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto lhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not rhs) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens,
                                         "invalid right-hand side operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *lhs);
                check_use_of_register(register_usage_profile, *rhs);

                check_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile, *lhs);
                check_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile, *rhs);

                auto val = Register(*result);
                val.value_type = register_usage_profile.at(*lhs).second;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == TEXT) {
                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto val = Register{};
                val.index = operand->index;
                val.register_set = operand->rss;
                val.value_type = viua::internals::ValueTypes::TEXT;

                register_usage_profile.define(val, operand->tokens.at(0));
            } else if (opcode == PRINT) {
                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *operand);

                register_usage_profile.use(Register(*operand), operand->tokens.at(0));
            }
        }

        for (const auto& each : register_usage_profile) {
            if (not register_usage_profile.used(each.first)) {
                ostringstream msg;
                msg << "unused value in register " << str::enquote(to_string(each.first.index));
                if (register_usage_profile.index_to_name.count(each.first.index)) {
                    msg << " (named "
                        << str::enquote(register_usage_profile.index_to_name.at(each.first.index)) << ')';
                } else {
                    msg << " (not named)";
                }

                throw InvalidSyntax(each.second.first, msg.str());
            }
        }
    });
}
