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
    pair<bool, Token> inferred = {false, {}};

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
    map<Register, pair<Token, Register>> defined_registers;

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

    auto define(const Register r, const Token t) -> void {
        defined_registers.insert_or_assign(r, pair<Token, Register>(t, r));
    }
    auto defined(const Register r) const -> bool { return defined_registers.count(r); }
    auto defined_where(const Register r) const -> Token { return defined_registers.at(r).first; }

    auto infer(const Register r, const viua::internals::ValueTypes value_type_id, const Token& t) -> void {
        auto reg = at(r);
        reg.second.value_type = value_type_id;
        reg.second.inferred = {true, t};
        define(reg.second, reg.first);
    }

    auto use(const Register r, const Token t) -> void { used_registers[r] = t; }
    auto used(const Register r) const -> bool { return used_registers.count(r); }

    auto at(const Register r) const -> const decltype(defined_registers)::mapped_type {
        return defined_registers.at(r);
    }

    auto erase(const Register r, const Token& token) -> void {
        erased_registers.emplace(r, token);
        defined_registers.erase(defined_registers.find(r));
    }
    auto erased(const Register r) const -> bool { return (erased_registers.count(r) == 1); }
    auto erased_where(const Register r) const -> Token { return erased_registers.at(r); }

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
static auto check_if_name_resolved(const RegisterUsageProfile& rup, const RegisterIndex r) -> void {
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
}
static auto check_use_of_register(RegisterUsageProfile& rup,
                                  viua::assembler::frontend::parser::RegisterIndex r) {
    check_if_name_resolved(rup, r);
    if (not rup.defined(Register(r))) {
        ostringstream msg;
        msg << "use of empty register " << str::enquote(to_string(r.index));
        if (rup.index_to_name.count(r.index)) {
            msg << " (named " << str::enquote(rup.index_to_name.at(r.index)) << ')';
        } else {
            msg << " (not named)";
        }
        auto error = TracedSyntaxError{}.append(InvalidSyntax(r.tokens.at(0), msg.str()));

        if (rup.erased(Register(r))) {
            error.append(InvalidSyntax(rup.erased_where(Register(r)), "").note("erased here:"));
        }

        throw error;
    }
    rup.use(Register(r), r.tokens.at(0));
}

using ValueTypes = viua::internals::ValueTypes;
using ValueTypesType = viua::internals::ValueTypesType;
static auto operator|(const ValueTypes lhs, const ValueTypes rhs) -> ValueTypes {
    // FIXME find out if it is possible to remove the outermost static_cast<>
    return static_cast<ValueTypes>(static_cast<ValueTypesType>(lhs) | static_cast<ValueTypesType>(rhs));
}
static auto operator&(const ValueTypes lhs, const ValueTypes rhs) -> ValueTypes {
    // FIXME find out if it is possible to remove the outermost static_cast<>
    return static_cast<ValueTypes>(static_cast<ValueTypesType>(lhs) & static_cast<ValueTypesType>(rhs));
}
static auto operator^(const ValueTypes lhs, const ValueTypes rhs) -> ValueTypes {
    // FIXME find out if it is possible to remove the outermost static_cast<>
    return static_cast<ValueTypes>(static_cast<ValueTypesType>(lhs) ^ static_cast<ValueTypesType>(rhs));
}
static auto operator!(const ValueTypes v) -> bool { return not static_cast<ValueTypesType>(v); }

auto value_type_names = map<ValueTypes, string>{
    {
        /*
         * Making this "value" instead of "undefined" lets us build error messages for unused registers.
         * They "unused value" can become "unused number" or "unused boolean", and
         * if the SA could not infer the type for the value we will not print a "unused undefined" (what
         * would that even mean?) message, but "unused value".
         */
        ValueTypes::UNDEFINED, "value"s,
    },
    {
        ValueTypes::VOID, "void"s,
    },
    {
        ValueTypes::INTEGER, "integer"s,
    },
    {
        ValueTypes::FLOAT, "float"s,
    },
    {
        ValueTypes::NUMBER, "number"s,
    },
    {
        ValueTypes::BOOLEAN, "boolean"s,
    },
    {
        ValueTypes::TEXT, "text"s,
    },
    {
        ValueTypes::STRING, "string"s,
    },
    {
        ValueTypes::VECTOR, "vector"s,
    },
    {
        ValueTypes::BITS, "bits"s,
    },
};
static auto to_string(ValueTypes value_type_id) -> string {
    auto has_pointer = not not(value_type_id & ValueTypes::POINTER);
    if (has_pointer) {
        value_type_id = (value_type_id ^ ValueTypes::POINTER);
    }

    return (has_pointer ? "pointer to "s : ""s) + value_type_names.at(value_type_id);
}

template<viua::internals::ValueTypes expected_type>
static auto assert_type_of_register(RegisterUsageProfile& register_usage_profile,
                                    const RegisterIndex& register_index) -> void {
    auto actual_type = register_usage_profile.at(Register(register_index)).second.value_type;

    auto access_via_pointer_dereference =
        (register_index.as == viua::internals::AccessSpecifier::POINTER_DEREFERENCE);
    if (access_via_pointer_dereference) {
        /*
         * Throw only if the type is not UNDEFINED.
         * If the type is UNDEFINED let the inferencer do its job.
         */
        if ((actual_type != ValueTypes::UNDEFINED) and not(actual_type & ValueTypes::POINTER)) {
            auto error =
                TracedSyntaxError{}
                    .append(InvalidSyntax(register_index.tokens.at(0),
                                          "invalid type of value contained in register for this access type")
                                .note("need pointer, got " + to_string(actual_type)))
                    .append(InvalidSyntax(register_usage_profile.defined_where(Register(register_index)), "")
                                .note("register defined here"));
            throw error;
        }

        /*
         * Modify types only if they are defined.
         * Tinkering with UNDEFINEDs will prevent inferencer from kicking in.
         */
        if (actual_type != ValueTypes::UNDEFINED) {
            actual_type = (actual_type ^ ValueTypes::POINTER);
        }
    }

    if (actual_type == ValueTypes::UNDEFINED) {
        auto inferred_type =
            (expected_type | (access_via_pointer_dereference ? ValueTypes::POINTER : ValueTypes::UNDEFINED));
        register_usage_profile.infer(Register(register_index), inferred_type, register_index.tokens.at(0));
        return;
    }

    if (expected_type == ValueTypes::UNDEFINED) {
        return;
    }
    if (not(actual_type & expected_type)) {
        auto error =
            TracedSyntaxError{}
                .append(
                    InvalidSyntax(register_index.tokens.at(0), "invalid type of value contained in register")
                        .note("expected " + to_string(expected_type) + ", got " + to_string(actual_type)))
                .append(InvalidSyntax(register_usage_profile.defined_where(Register(register_index)), "")
                            .note("register defined here"));
        if (auto r = register_usage_profile.at(Register(register_index)).second; r.inferred.first) {
            error.append(InvalidSyntax(r.inferred.second, "")
                             .note("type inferred here")
                             .aside("deduced type is '" + to_string(r.value_type) + "'"));
        }
        throw error;
    }
}

static auto erase_if_direct_access(RegisterUsageProfile& register_usage_profile, RegisterIndex* r,
                                   viua::assembler::frontend::parser::Instruction* instruction) {
    if (r->as == viua::internals::AccessSpecifier::DIRECT) {
        register_usage_profile.erase(Register(*r), instruction->tokens.at(0));
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
            if (opcode == IZERO) {
                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *operand);

                auto val = Register{};
                val.index = operand->index;
                val.register_set = operand->rss;
                val.value_type = viua::internals::ValueTypes::INTEGER;
                register_usage_profile.define(val, operand->tokens.at(0));
            } else if (opcode == ISTORE) {
                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *operand);

                auto val = Register{};
                val.index = operand->index;
                val.register_set = operand->rss;
                val.value_type = viua::internals::ValueTypes::INTEGER;
                register_usage_profile.define(val, operand->tokens.at(0));
            } else if (opcode == IINC or opcode == IDEC) {
                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *operand);

                check_use_of_register(register_usage_profile, *operand);
                assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile,
                                                                              *operand);
            } else if (opcode == FSTORE) {
                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *operand);

                auto val = Register{};
                val.index = operand->index;
                val.register_set = operand->rss;
                val.value_type = viua::internals::ValueTypes::FLOAT;
                register_usage_profile.define(val, operand->tokens.at(0));
            } else if (opcode == ITOF) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *operand);

                assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile,
                                                                              *operand);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::FLOAT;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == FTOI) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *operand);

                assert_type_of_register<viua::internals::ValueTypes::FLOAT>(register_usage_profile, *operand);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::INTEGER;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == STOI) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *operand);

                assert_type_of_register<viua::internals::ValueTypes::TEXT>(register_usage_profile, *operand);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::INTEGER;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == STOF) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *operand);

                assert_type_of_register<viua::internals::ValueTypes::TEXT>(register_usage_profile, *operand);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::FLOAT;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == ADD or opcode == SUB or opcode == MUL or opcode == DIV) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not rhs) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens,
                                         "invalid right-hand side operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *lhs);
                check_use_of_register(register_usage_profile, *rhs);

                assert_type_of_register<viua::internals::ValueTypes::NUMBER>(register_usage_profile, *lhs);
                assert_type_of_register<viua::internals::ValueTypes::NUMBER>(register_usage_profile, *rhs);

                auto val = Register(*result);
                val.value_type = register_usage_profile.at(*lhs).second.value_type;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == LT or opcode == LTE or opcode == GT or opcode == GTE or opcode == EQ) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not rhs) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens,
                                         "invalid right-hand side operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *lhs);
                check_use_of_register(register_usage_profile, *rhs);

                assert_type_of_register<viua::internals::ValueTypes::NUMBER>(register_usage_profile, *lhs);
                assert_type_of_register<viua::internals::ValueTypes::NUMBER>(register_usage_profile, *rhs);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::BOOLEAN;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == STRSTORE) {
                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *operand);

                auto val = Register{};
                val.index = operand->index;
                val.register_set = operand->rss;
                val.value_type = viua::internals::ValueTypes::STRING;

                register_usage_profile.define(val, operand->tokens.at(0));
            } else if (opcode == STREQ) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not rhs) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens,
                                         "invalid right-hand side operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *lhs);
                check_use_of_register(register_usage_profile, *rhs);

                assert_type_of_register<viua::internals::ValueTypes::STRING>(register_usage_profile, *lhs);
                assert_type_of_register<viua::internals::ValueTypes::STRING>(register_usage_profile, *rhs);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::BOOLEAN;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == TEXT) {
                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *operand);

                auto val = Register{};
                val.index = operand->index;
                val.register_set = operand->rss;
                val.value_type = viua::internals::ValueTypes::TEXT;

                register_usage_profile.define(val, operand->tokens.at(0));
            } else if (opcode == TEXTEQ) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not rhs) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens,
                                         "invalid right-hand side operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *lhs);
                check_use_of_register(register_usage_profile, *rhs);

                assert_type_of_register<viua::internals::ValueTypes::TEXT>(register_usage_profile, *lhs);
                assert_type_of_register<viua::internals::ValueTypes::TEXT>(register_usage_profile, *rhs);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::BOOLEAN;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == TEXTAT) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto key = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not key) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                check_use_of_register(register_usage_profile, *key);

                assert_type_of_register<viua::internals::ValueTypes::TEXT>(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile, *key);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::TEXT;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == TEXTSUB) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto key_begin = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not key_begin) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto key_end = dynamic_cast<RegisterIndex*>(instruction->operands.at(3).get());
                if (not key_end) {
                    throw invalid_syntax(instruction->operands.at(3)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                check_use_of_register(register_usage_profile, *key_begin);
                check_use_of_register(register_usage_profile, *key_end);

                assert_type_of_register<viua::internals::ValueTypes::TEXT>(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile,
                                                                              *key_begin);
                assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile,
                                                                              *key_end);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::TEXT;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == TEXTLENGTH) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *operand);

                assert_type_of_register<viua::internals::ValueTypes::TEXT>(register_usage_profile, *operand);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::INTEGER;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == TEXTCOMMONPREFIX) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not rhs) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens,
                                         "invalid right-hand side operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *lhs);
                check_use_of_register(register_usage_profile, *rhs);

                assert_type_of_register<viua::internals::ValueTypes::TEXT>(register_usage_profile, *lhs);
                assert_type_of_register<viua::internals::ValueTypes::TEXT>(register_usage_profile, *rhs);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::INTEGER;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == TEXTCOMMONSUFFIX) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not rhs) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens,
                                         "invalid right-hand side operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *lhs);
                check_use_of_register(register_usage_profile, *rhs);

                assert_type_of_register<viua::internals::ValueTypes::TEXT>(register_usage_profile, *lhs);
                assert_type_of_register<viua::internals::ValueTypes::TEXT>(register_usage_profile, *rhs);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::INTEGER;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == TEXTCONCAT) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not rhs) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens,
                                         "invalid right-hand side operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *lhs);
                check_use_of_register(register_usage_profile, *rhs);

                assert_type_of_register<viua::internals::ValueTypes::TEXT>(register_usage_profile, *lhs);
                assert_type_of_register<viua::internals::ValueTypes::TEXT>(register_usage_profile, *rhs);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::TEXT;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == VEC) {
                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *operand);

                auto val = Register{};
                val.index = operand->index;
                val.register_set = operand->rss;
                val.value_type = viua::internals::ValueTypes::VECTOR;
                register_usage_profile.define(val, operand->tokens.at(0));
            } else if (opcode == VINSERT) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *result);
                assert_type_of_register<viua::internals::ValueTypes::VECTOR>(register_usage_profile, *result);

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto key = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not key) {
                    if (not dynamic_cast<VoidLiteral*>(instruction->operands.at(2).get())) {
                        throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                            .note("expected register index or void");
                    }
                }

                check_use_of_register(register_usage_profile, *source);

                if (key) {
                    check_use_of_register(register_usage_profile, *key);
                    assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile,
                                                                                  *key);
                }

                erase_if_direct_access(register_usage_profile, source, instruction);
            } else if (opcode == VPUSH) {
                auto target = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *target);
                assert_type_of_register<viua::internals::ValueTypes::VECTOR>(register_usage_profile, *target);

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                erase_if_direct_access(register_usage_profile, source, instruction);
            } else if (opcode == VPOP) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    if (not dynamic_cast<VoidLiteral*>(instruction->operands.at(0).get())) {
                        throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                            .note("expected register index or void");
                    }
                }

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::VECTOR>(register_usage_profile, *source);

                auto key = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not key) {
                    if (not dynamic_cast<VoidLiteral*>(instruction->operands.at(2).get())) {
                        throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                            .note("expected register index or void");
                    }
                }

                if (key) {
                    check_use_of_register(register_usage_profile, *key);
                    assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile,
                                                                                  *key);
                }

                if (result) {
                    auto val = Register(*result);
                    register_usage_profile.define(val, result->tokens.at(0));
                }
            } else if (opcode == VAT) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index or void");
                }

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::VECTOR>(register_usage_profile, *source);

                auto key = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not key) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                        .note("expected register index or void");
                }

                check_use_of_register(register_usage_profile, *key);
                assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile, *key);

                auto val = Register(*result);
                val.value_type = ValueTypes::POINTER;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == VLEN) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index or void");
                }

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::VECTOR>(register_usage_profile, *source);

                auto val = Register(*result);
                val.value_type = ValueTypes::INTEGER;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == NOT) {
                auto target = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *target);

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);

                auto val = Register(*target);
                val.value_type = viua::internals::ValueTypes::BOOLEAN;
                register_usage_profile.define(val, target->tokens.at(0));
            } else if (opcode == AND or opcode == OR) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not rhs) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens,
                                         "invalid right-hand side operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *lhs);
                check_use_of_register(register_usage_profile, *rhs);

                auto val = Register(*result);
                val.value_type = ValueTypes::BOOLEAN;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == BITS) {
                auto target = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *target);

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    if (not dynamic_cast<BitsLiteral*>(instruction->operands.at(1).get())) {
                        throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                            .note("expected register index or bits literal");
                    }
                }

                if (source) {
                    check_use_of_register(register_usage_profile, *source);
                    assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile,
                                                                                  *source);
                }

                auto val = Register{};
                val.index = target->index;
                val.register_set = target->rss;
                val.value_type = viua::internals::ValueTypes::BITS;
                register_usage_profile.define(val, target->tokens.at(0));
            } else if (opcode == BITAND or opcode == BITOR or opcode == BITXOR) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not rhs) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens,
                                         "invalid right-hand side operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *lhs);
                check_use_of_register(register_usage_profile, *rhs);

                assert_type_of_register<viua::internals::ValueTypes::BITS>(register_usage_profile, *lhs);
                assert_type_of_register<viua::internals::ValueTypes::BITS>(register_usage_profile, *rhs);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::BITS;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == BITNOT) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *operand);

                assert_type_of_register<ValueTypes::VECTOR>(register_usage_profile, *operand);

                auto val = Register(*result);
                val.value_type = ValueTypes::VECTOR;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == BITAT) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto key = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not key) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                check_use_of_register(register_usage_profile, *key);

                assert_type_of_register<viua::internals::ValueTypes::BITS>(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile, *key);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::BOOLEAN;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == BITSET) {
                auto target = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *target);
                assert_type_of_register<viua::internals::ValueTypes::BITS>(register_usage_profile, *target);

                auto index = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not index) {
                    if (not dynamic_cast<BitsLiteral*>(instruction->operands.at(1).get())) {
                        throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                            .note("expected register index or bits literal");
                    }
                }

                check_use_of_register(register_usage_profile, *index);
                assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile, *index);

                auto value = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not value) {
                    if (not dynamic_cast<BooleanLiteral*>(instruction->operands.at(2).get())) {
                        throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                            .note("expected register index or boolean literal");
                    }
                }

                check_use_of_register(register_usage_profile, *value);
                assert_type_of_register<viua::internals::ValueTypes::BOOLEAN>(register_usage_profile, *value);
            } else if (opcode == SHL or opcode == SHR or opcode == ASHL or opcode == ASHR) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    if (not dynamic_cast<VoidLiteral*>(instruction->operands.at(0).get())) {
                        throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                            .note("expected register index or void");
                    }
                }

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::BITS>(register_usage_profile, *source);

                auto offset = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not offset) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *offset);
                assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile,
                                                                              *offset);

                if (result) {
                    auto val = Register(*result);
                    val.value_type = ValueTypes::BITS;
                    register_usage_profile.define(val, result->tokens.at(0));
                }
            } else if (opcode == ROL or opcode == ROR) {
                auto target = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto offset = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not offset) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *target);
                check_use_of_register(register_usage_profile, *offset);

                assert_type_of_register<viua::internals::ValueTypes::BITS>(register_usage_profile, *target);
                assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile,
                                                                              *offset);
            } else if (opcode == COPY) {
                auto target = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *source);

                auto val = Register(*target);
                val.value_type = register_usage_profile.at(*source).second.value_type;
                register_usage_profile.define(val, target->tokens.at(0));
            } else if (opcode == MOVE) {
                auto target = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *source);

                auto val = Register(*target);
                val.value_type = register_usage_profile.at(*source).second.value_type;
                register_usage_profile.define(val, target->tokens.at(0));

                erase_if_direct_access(register_usage_profile, source, instruction);
            } else if (opcode == PTR) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *operand);

                auto val = Register(*result);
                val.value_type = (register_usage_profile.at(Register(*operand)).second.value_type |
                                  viua::internals::ValueTypes::POINTER);
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == SWAP) {
                auto target = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto source = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *source);

                auto val_target = Register(*target);
                val_target.value_type = register_usage_profile.at(*source).second.value_type;

                auto val_source = Register(*source);
                val_source.value_type = register_usage_profile.at(*target).second.value_type;

                register_usage_profile.define(val_target, target->tokens.at(0));
                register_usage_profile.define(val_source, source->tokens.at(0));
            } else if (opcode == PRINT) {
                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *operand);

                // This type assertion is here because we can infer the register to contain a pointer, and
                // even if we can't be sure if it's a pointer to text or integer, the fact that a register
                // holds a *pointer* is valuable on its own.
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *operand);

                register_usage_profile.use(Register(*operand), operand->tokens.at(0));
            } else if (opcode == ARG) {
                if (dynamic_cast<VoidLiteral*>(instruction->operands.at(0).get())) {
                    continue;
                }

                auto target = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index or void");
                }

                check_if_name_resolved(register_usage_profile, *target);

                auto val = Register(*target);
                register_usage_profile.define(val, target->tokens.at(0));
            }
        }

        for (const auto& each : register_usage_profile) {
            if (not each.first.index) {
                /*
                 * Registers with index 0 do not take part in the "unused" analysis, because
                 * they are used to store return values of functions.
                 * This means that they *MUST* be defined, but *MAY* stay unused.
                 */
                continue;
            }
            if (not register_usage_profile.used(each.first)) {
                ostringstream msg;
                msg << "unused " + to_string(each.second.second.value_type) + " in register "
                    << str::enquote(to_string(each.first.index));
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
