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


using viua::cg::lex::InvalidSyntax;
using viua::cg::lex::Token;
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

struct Closure {
    string name;
    map<Register, pair<Token, Register>> defined_registers;

    auto define(const Register r, const Token t) -> void {
        defined_registers.insert_or_assign(r, pair<Token, Register>(t, r));
    }

    Closure() : name("") {}
    Closure(string n) : name(n) {}
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


using viua::internals::RegisterSets;
auto register_set_names = map<RegisterSets, string>{
    {
        // FIXME 'current' as a register set name should be deprecated.
        RegisterSets::CURRENT,
        "current"s,
    },
    {
        RegisterSets::GLOBAL,
        "global"s,
    },
    {
        RegisterSets::STATIC,
        "static"s,
    },
    {
        RegisterSets::LOCAL,
        "local"s,
    },
};
static auto to_string(RegisterSets register_set_id) { return register_set_names.at(register_set_id); }


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
static auto maybe_mistyped_register_set_helper(RegisterUsageProfile& rup,
                                               viua::assembler::frontend::parser::RegisterIndex r,
                                               TracedSyntaxError& error, RegisterSets rs_id) -> bool {
    if (r.rss != rs_id) {
        auto val = Register{};
        val.index = r.index;
        val.register_set = rs_id;
        if (rup.defined(val)) {
            error.errors.back().aside("did you mean " + to_string(rs_id) + " register " + to_string(r.index) +
                                      "?");
            error.append(
                InvalidSyntax(rup.defined_where(val), "")
                    .note(to_string(rs_id) + " register " + to_string(r.index) + " was defined here"));
            return true;
        }
    }
    return false;
}
static auto maybe_mistyped_register_set(RegisterUsageProfile& rup,
                                        viua::assembler::frontend::parser::RegisterIndex r,
                                        TracedSyntaxError& error) -> void {
    if (maybe_mistyped_register_set_helper(rup, r, error, RegisterSets::LOCAL)) {
        return;
    }
    if (maybe_mistyped_register_set_helper(rup, r, error, RegisterSets::STATIC)) {
        return;
    }
}
static auto check_use_of_register(RegisterUsageProfile& rup,
                                  viua::assembler::frontend::parser::RegisterIndex r,
                                  const string error_core_msg = "use of") {
    check_if_name_resolved(rup, r);
    if (not rup.defined(Register(r))) {
        auto empty_or_erased = (rup.erased(Register(r)) ? "erased"s : "empty"s);

        ostringstream msg;
        msg << error_core_msg << ' ' << empty_or_erased << ' ' << to_string(r.rss) << " register "
            << str::enquote(to_string(r.index));
        if (rup.index_to_name.count(r.index)) {
            msg << " (named " << str::enquote(rup.index_to_name.at(r.index)) << ')';
        }
        auto error = TracedSyntaxError{}.append(InvalidSyntax(r.tokens.at(0), msg.str()));

        if (rup.erased(Register(r))) {
            error.append(InvalidSyntax(rup.erased_where(Register(r)), "").note("erased here:"));
        }

        maybe_mistyped_register_set(rup, r, error);

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
        ValueTypes::UNDEFINED,
        "value"s,
    },
    {
        ValueTypes::VOID,
        "void"s,
    },
    {
        ValueTypes::INTEGER,
        "integer"s,
    },
    {
        ValueTypes::FLOAT,
        "float"s,
    },
    {
        ValueTypes::NUMBER,
        "number"s,
    },
    {
        ValueTypes::BOOLEAN,
        "boolean"s,
    },
    {
        ValueTypes::TEXT,
        "text"s,
    },
    {
        ValueTypes::STRING,
        "string"s,
    },
    {
        ValueTypes::VECTOR,
        "vector"s,
    },
    {
        ValueTypes::BITS,
        "bits"s,
    },
    {
        ValueTypes::CLOSURE,
        "closure"s,
    },
    {
        ValueTypes::FUNCTION,
        "function"s,
    },
    {
        ValueTypes::INVOCABLE,
        "invocable"s,
    },
    {
        ValueTypes::ATOM,
        "atom"s,
    },
    {
        ValueTypes::PID,
        "pid"s,
    },
    {
        ValueTypes::STRUCT,
        "struct"s,
    },
    {
        ValueTypes::OBJECT,
        "object"s,
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
                                .note("need pointer to " + to_string(expected_type) + ", got " +
                                      to_string(actual_type)))
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
                                   viua::assembler::frontend::parser::Instruction const* const instruction) {
    if (r->as == viua::internals::AccessSpecifier::DIRECT) {
        register_usage_profile.erase(Register(*r), instruction->tokens.at(0));
    }
}

static auto map_names_to_register_indexes(RegisterUsageProfile& register_usage_profile,
                                          const InstructionsBlock& ib) -> void {
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

        if (register_usage_profile.name_to_index.count(name)) {
            throw InvalidSyntax{directive->tokens.at(2), "register name already taken: " + name}.add(
                directive->tokens.at(0));
        }

        register_usage_profile.name_to_index[name] = index;
        register_usage_profile.index_to_name[index] = name;
    }
}
static auto check_for_unused_registers(const RegisterUsageProfile& register_usage_profile) -> void {
    for (const auto& each : register_usage_profile) {
        if (not each.first.index) {
            /*
             * Registers with index 0 do not take part in the "unused" analysis, because
             * they are used to store return values of functions.
             * This means that they *MUST* be defined, but *MAY* stay unused.
             */
            continue;
        }
        // FIXME Implement the .unused: directive, and [[maybe_unused]] attribute (later).
        if (not register_usage_profile.used(each.first)) {
            ostringstream msg;
            msg << "unused " + to_string(each.second.second.value_type) + " in register "
                << str::enquote(to_string(each.first.index));
            if (register_usage_profile.index_to_name.count(each.first.index)) {
                msg << " (named " << str::enquote(register_usage_profile.index_to_name.at(each.first.index))
                    << ')';
            }

            throw viua::cg::lex::UnusedValue(each.second.first, msg.str());
        }
    }
}
using InstructionIndex = InstructionsBlock::size_type;
static auto check_register_usage_for_instruction_block_impl(RegisterUsageProfile&, const ParsedSource&,
                                                            const InstructionsBlock&, InstructionIndex,
                                                            InstructionIndex) -> void;
static auto check_closure_instantiations(const RegisterUsageProfile& register_usage_profile,
                                         const ParsedSource& ps,
                                         const map<Register, Closure>& created_closures) -> void {
    for (const auto& each : created_closures) {
        RegisterUsageProfile closure_register_usage_profile;
        const auto& fn = *find_if(ps.functions.begin(), ps.functions.end(),
                                  [&each](const InstructionsBlock& b) { return b.name == each.second.name; });
        for (auto& captured_value : each.second.defined_registers) {
            closure_register_usage_profile.define(captured_value.second.second, captured_value.second.first);
        }

        try {
            map_names_to_register_indexes(closure_register_usage_profile, fn);
            check_register_usage_for_instruction_block_impl(closure_register_usage_profile, ps, fn, 0,
                                                            static_cast<InstructionIndex>(-1));
        } catch (InvalidSyntax& e) {
            throw TracedSyntaxError{}
                .append(e)
                .append(InvalidSyntax{fn.name, "in a closure defined here:"})
                .append(InvalidSyntax{register_usage_profile.defined_where(each.first),
                                      "when instantiated here:"});
        } catch (TracedSyntaxError& e) {
            throw e.append(InvalidSyntax{fn.name, "in a closure defined here:"})
                .append(InvalidSyntax{register_usage_profile.defined_where(each.first),
                                      "when instantiated here:"});
        }
    }
}

static auto get_line_index_of_instruction(const InstructionIndex n, const InstructionsBlock& ib)
    -> InstructionIndex {
    auto left = n;
    auto i = InstructionIndex{0};
    for (; left and i < ib.body.size(); ++i) {
        auto instruction = dynamic_cast<viua::assembler::frontend::parser::Instruction*>(ib.body.at(i).get());
        if (instruction) {
            --left;
        }
    }
    return i;
}
template<typename T>
static auto get_operand(viua::assembler::frontend::parser::Instruction const& instruction,
                        size_t operand_index) -> T* {
    if (operand_index >= instruction.operands.size()) {
        throw InvalidSyntax{instruction.tokens.at(0), "not enough operands"}.note("when extracting operand " +
                                                                                  to_string(operand_index));
    }
    T* operand = dynamic_cast<T*>(instruction.operands.at(operand_index).get());
    return operand;
}
template<typename T>
static auto get_input_operand(viua::assembler::frontend::parser::Instruction const& instruction,
                              size_t operand_index) -> T* {
    auto operand = get_operand<T>(instruction, operand_index);
    if ((not operand) and dynamic_cast<VoidLiteral*>(instruction.operands.at(operand_index).get())) {
        throw InvalidSyntax{instruction.operands.at(operand_index)->tokens.at(0),
                            "use of void as input register"};
    }
    return operand;
}
static auto check_register_usage_for_instruction_block_impl(RegisterUsageProfile& register_usage_profile,
                                                            const ParsedSource& ps,
                                                            const InstructionsBlock& ib, InstructionIndex i,
                                                            InstructionIndex mnemonic_counter) -> void {
    map<Register, Closure> created_closures;

    for (; i < ib.body.size(); ++i) {
        const auto& line = ib.body.at(i);
        auto instruction = dynamic_cast<viua::assembler::frontend::parser::Instruction*>(line.get());
        if (not instruction) {
            continue;
        }
        ++mnemonic_counter;

        using viua::assembler::frontend::parser::RegisterIndex;
        auto opcode = instruction->opcode;

        try {
            if (opcode == IZERO) {
                auto operand = get_operand<RegisterIndex>(*instruction, 0);
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
                auto operand = get_operand<RegisterIndex>(*instruction, 0);
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
                auto operand = get_operand<RegisterIndex>(*instruction, 0);
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *operand);

                check_use_of_register(register_usage_profile, *operand);
                assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile,
                                                                              *operand);
            } else if (opcode == FSTORE) {
                auto operand = get_operand<RegisterIndex>(*instruction, 0);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = get_operand<RegisterIndex>(*instruction, 1);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = get_operand<RegisterIndex>(*instruction, 1);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = get_operand<RegisterIndex>(*instruction, 1);
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *operand);

                assert_type_of_register<viua::internals::ValueTypes::STRING>(register_usage_profile,
                                                                             *operand);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::INTEGER;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == STOF) {
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = get_operand<RegisterIndex>(*instruction, 1);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = get_operand<RegisterIndex>(*instruction, 1);
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = get_operand<RegisterIndex>(*instruction, 2);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = get_operand<RegisterIndex>(*instruction, 1);
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = get_operand<RegisterIndex>(*instruction, 2);
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
                auto operand = get_operand<RegisterIndex>(*instruction, 0);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = get_operand<RegisterIndex>(*instruction, 1);
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = get_operand<RegisterIndex>(*instruction, 2);
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
                auto operand = get_operand<RegisterIndex>(*instruction, 0);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = get_operand<RegisterIndex>(*instruction, 1);
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = get_operand<RegisterIndex>(*instruction, 2);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto key = get_operand<RegisterIndex>(*instruction, 2);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto key_begin = get_operand<RegisterIndex>(*instruction, 2);
                if (not key_begin) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto key_end = get_operand<RegisterIndex>(*instruction, 3);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = get_operand<RegisterIndex>(*instruction, 1);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = get_operand<RegisterIndex>(*instruction, 1);
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = get_operand<RegisterIndex>(*instruction, 2);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = get_operand<RegisterIndex>(*instruction, 1);
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = get_operand<RegisterIndex>(*instruction, 2);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = get_operand<RegisterIndex>(*instruction, 1);
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = get_operand<RegisterIndex>(*instruction, 2);
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
                auto operand = get_operand<RegisterIndex>(*instruction, 0);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *result);
                assert_type_of_register<viua::internals::ValueTypes::VECTOR>(register_usage_profile, *result);

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto key = get_operand<RegisterIndex>(*instruction, 2);
                if (not key) {
                    if (not get_operand<VoidLiteral>(*instruction, 2)) {
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
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *target);
                assert_type_of_register<viua::internals::ValueTypes::VECTOR>(register_usage_profile, *target);

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                erase_if_direct_access(register_usage_profile, source, instruction);
            } else if (opcode == VPOP) {
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    if (not get_operand<VoidLiteral>(*instruction, 0)) {
                        throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                            .note("expected register index or void");
                    }
                }

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::VECTOR>(register_usage_profile, *source);

                auto key = get_operand<RegisterIndex>(*instruction, 2);
                if (not key) {
                    if (not get_operand<VoidLiteral>(*instruction, 2)) {
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index or void");
                }

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::VECTOR>(register_usage_profile, *source);

                auto key = get_operand<RegisterIndex>(*instruction, 2);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index or void");
                }

                auto source = get_operand<RegisterIndex>(*instruction, 1);
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
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *target);

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);

                auto val = Register(*target);
                val.value_type = viua::internals::ValueTypes::BOOLEAN;
                register_usage_profile.define(val, target->tokens.at(0));
            } else if (opcode == AND or opcode == OR) {
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = get_operand<RegisterIndex>(*instruction, 1);
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = get_operand<RegisterIndex>(*instruction, 2);
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
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *target);

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    if (not get_operand<BitsLiteral>(*instruction, 1)) {
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = get_operand<RegisterIndex>(*instruction, 1);
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = get_operand<RegisterIndex>(*instruction, 2);
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
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = get_operand<RegisterIndex>(*instruction, 1);
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *operand);

                assert_type_of_register<ValueTypes::BITS>(register_usage_profile, *operand);

                auto val = Register(*result);
                val.value_type = ValueTypes::BITS;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == BITAT) {
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto key = get_operand<RegisterIndex>(*instruction, 2);
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
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *target);
                assert_type_of_register<viua::internals::ValueTypes::BITS>(register_usage_profile, *target);

                auto index = get_operand<RegisterIndex>(*instruction, 1);
                if (not index) {
                    if (not get_operand<BitsLiteral>(*instruction, 1)) {
                        throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                            .note("expected register index or bits literal");
                    }
                }

                check_use_of_register(register_usage_profile, *index);
                assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile, *index);

                auto value = get_operand<RegisterIndex>(*instruction, 2);
                if (not value) {
                    if (not get_operand<BooleanLiteral>(*instruction, 2)) {
                        throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                            .note("expected register index or boolean literal");
                    }
                }

                if (value) {
                    check_use_of_register(register_usage_profile, *value);
                    assert_type_of_register<viua::internals::ValueTypes::BOOLEAN>(register_usage_profile,
                                                                                  *value);
                }
            } else if (opcode == SHL or opcode == SHR or opcode == ASHL or opcode == ASHR) {
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    if (not get_operand<VoidLiteral>(*instruction, 0)) {
                        throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                            .note("expected register index or void");
                    }
                }

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::BITS>(register_usage_profile, *source);

                auto offset = get_operand<RegisterIndex>(*instruction, 2);
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
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto offset = get_operand<RegisterIndex>(*instruction, 1);
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
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source, "copy from");
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *source);

                auto val = Register(*target);
                val.value_type = register_usage_profile.at(*source).second.value_type;
                register_usage_profile.define(val, target->tokens.at(0));
            } else if (opcode == MOVE) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source, "move from");
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *source);

                auto val = Register(*target);
                val.value_type = register_usage_profile.at(*source).second.value_type;
                register_usage_profile.define(val, target->tokens.at(0));

                erase_if_direct_access(register_usage_profile, source, instruction);
            } else if (opcode == PTR) {
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto operand = get_operand<RegisterIndex>(*instruction, 1);
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *operand, "pointer from");

                auto val = Register(*result);
                val.value_type = (register_usage_profile.at(Register(*operand)).second.value_type |
                                  viua::internals::ValueTypes::POINTER);
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == SWAP) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *target, "swap with");
                if (target->as == viua::internals::AccessSpecifier::POINTER_DEREFERENCE) {
                    throw InvalidSyntax(target->tokens.at(0), "invalid access mode")
                        .note("can only swap using direct access mode")
                        .aside("did you mean '%" + target->tokens.at(0).str().substr(1) + "'?");
                }

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source, "swap with");
                if (source->as == viua::internals::AccessSpecifier::POINTER_DEREFERENCE) {
                    throw InvalidSyntax(source->tokens.at(0), "invalid access mode")
                        .note("can only swap using direct access mode")
                        .aside("did you mean '%" + source->tokens.at(0).str().substr(1) + "'?");
                }

                auto val_target = Register(*target);
                val_target.value_type = register_usage_profile.at(*source).second.value_type;

                auto val_source = Register(*source);
                val_source.value_type = register_usage_profile.at(*target).second.value_type;

                register_usage_profile.define(val_target, target->tokens.at(0));
                register_usage_profile.define(val_source, source->tokens.at(0));
            } else if (opcode == DELETE) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *target, "delete of");
                if (target->as != viua::internals::AccessSpecifier::DIRECT) {
                    throw InvalidSyntax(target->tokens.at(0), "invalid access mode")
                        .note("can only delete using direct access mode")
                        .aside("did you mean '%" + target->tokens.at(0).str().substr(1) + "'?");
                }
                register_usage_profile.erase(Register(*target), instruction->tokens.at(0));
            } else if (opcode == ISNULL) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                if (source->as == viua::internals::AccessSpecifier::POINTER_DEREFERENCE) {
                    throw InvalidSyntax(source->tokens.at(0), "invalid access mode")
                        .note("can only check using direct access mode")
                        .aside("did you mean '%" + source->tokens.at(0).str().substr(1) + "'?");
                }

                if (register_usage_profile.defined(Register{*source})) {
                    throw TracedSyntaxError{}
                        .append(InvalidSyntax{source->tokens.at(0),
                                              "useless check, register will always be defined"})
                        .append(InvalidSyntax{register_usage_profile.defined_where(Register{*source})}.note(
                            "register is defined here"));
                }

                auto val = Register(*target);
                val.value_type = ValueTypes::BOOLEAN;
                register_usage_profile.define(val, target->tokens.at(0));
            } else if (opcode == RESS) {
                // do nothing
            } else if (opcode == PRINT or opcode == ECHO) {
                auto operand = get_input_operand<RegisterIndex>(*instruction, 0);
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
            } else if (opcode == CAPTURE) {
                auto closure = get_operand<RegisterIndex>(*instruction, 0);
                if (not closure) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *closure);
                assert_type_of_register<viua::internals::ValueTypes::CLOSURE>(register_usage_profile,
                                                                              *closure);

                // this index is not verified because it is used as *the* index to use when
                // putting a value inside the closure
                auto index = get_operand<RegisterIndex>(*instruction, 1);
                if (not index) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto source = get_operand<RegisterIndex>(*instruction, 2);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *source);

                auto val = Register{};
                val.index = index->index;
                val.register_set = RegisterSets::LOCAL;
                val.value_type = register_usage_profile.at(*source).second.value_type;
                created_closures.at(Register{*closure}).define(val, index->tokens.at(0));
            } else if (opcode == CAPTURECOPY) {
                auto closure = get_operand<RegisterIndex>(*instruction, 0);
                if (not closure) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *closure);
                assert_type_of_register<viua::internals::ValueTypes::CLOSURE>(register_usage_profile,
                                                                              *closure);

                // this index is not verified because it is used as *the* index to use when
                // putting a value inside the closure
                auto index = get_operand<RegisterIndex>(*instruction, 1);
                if (not index) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto source = get_operand<RegisterIndex>(*instruction, 2);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *source);

                auto val = Register{};
                val.index = index->index;
                val.register_set = RegisterSets::LOCAL;
                val.value_type = register_usage_profile.at(*source).second.value_type;
                created_closures.at(Register{*closure}).define(val, index->tokens.at(0));
            } else if (opcode == CAPTUREMOVE) {
                auto closure = get_operand<RegisterIndex>(*instruction, 0);
                if (not closure) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *closure);
                assert_type_of_register<viua::internals::ValueTypes::CLOSURE>(register_usage_profile,
                                                                              *closure);

                // this index is not verified because it is used as *the* index to use when
                // putting a value inside the closure
                auto index = get_operand<RegisterIndex>(*instruction, 1);
                if (not index) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                auto source = get_operand<RegisterIndex>(*instruction, 2);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *source);

                auto val = Register{};
                val.index = index->index;
                val.register_set = RegisterSets::LOCAL;
                val.value_type = register_usage_profile.at(*source).second.value_type;
                created_closures.at(Register{*closure}).define(val, index->tokens.at(0));

                erase_if_direct_access(register_usage_profile, source, instruction);
            } else if (opcode == CLOSURE) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *target);

                auto fn = get_operand<FunctionNameLiteral>(*instruction, 1);
                if (not fn) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected function name literal");
                }

                /*
                 * FIXME The SA should switch to verification of the closure after it has been
                 * created and all required values captured.
                 * The SA will need some guidance to discover the precise moment at which it can
                 * start checking the closure for correctness.
                 */

                auto val = Register{*target};
                val.value_type = ValueTypes::CLOSURE;
                register_usage_profile.define(val, target->tokens.at(0));

                created_closures[val] = Closure{fn->content};
            } else if (opcode == FUNCTION) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *target);

                if (not get_operand<FunctionNameLiteral>(*instruction, 1)) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected function name literal");
                }

                auto val = Register{*target};
                val.value_type = ValueTypes::FUNCTION;
                register_usage_profile.define(val, target->tokens.at(0));
            } else if (opcode == FRAME) {
                // do nothing, FRAMEs do not modify registers;
                // also, frame balance and arity is handled by verifier which runs before SA
            } else if (opcode == PARAM) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                if (target->as == viua::internals::AccessSpecifier::REGISTER_INDIRECT) {
                    auto r = *target;
                    r.rss = viua::internals::RegisterSets::LOCAL;
                    check_use_of_register(register_usage_profile, r);
                    assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile, r);
                }

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *source);
            } else if (opcode == PAMV) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }
                if (target->as == viua::internals::AccessSpecifier::REGISTER_INDIRECT) {
                    auto r = *target;
                    r.rss = viua::internals::RegisterSets::LOCAL;
                    check_use_of_register(register_usage_profile, r);
                    assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile, r);
                }

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *source);

                erase_if_direct_access(register_usage_profile, source, instruction);
            } else if (opcode == CALL) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    if (not get_operand<VoidLiteral>(*instruction, 0)) {
                        throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                            .note("expected register index or void");
                    }
                }

                if (target) {
                    check_if_name_resolved(register_usage_profile, *target);
                }

                auto fn = instruction->operands.at(1).get();
                if ((not dynamic_cast<AtomLiteral*>(fn)) and (not dynamic_cast<FunctionNameLiteral*>(fn)) and
                    (not dynamic_cast<RegisterIndex*>(fn))) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected function name, atom literal, or register index");
                }
                if (auto r = dynamic_cast<RegisterIndex*>(fn); r) {
                    check_use_of_register(register_usage_profile, *r, "call from");
                    assert_type_of_register<viua::internals::ValueTypes::INVOCABLE>(register_usage_profile,
                                                                                    *r);
                }

                if (target) {
                    register_usage_profile.define(Register{*target}, target->tokens.at(0));
                }
            } else if (opcode == TAILCALL) {
                auto fn = instruction->operands.at(0).get();
                if ((not dynamic_cast<AtomLiteral*>(fn)) and (not dynamic_cast<FunctionNameLiteral*>(fn)) and
                    (not dynamic_cast<RegisterIndex*>(fn))) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected function name, atom literal, or register index");
                }
                if (auto r = dynamic_cast<RegisterIndex*>(fn); r) {
                    check_use_of_register(register_usage_profile, *r);
                    assert_type_of_register<viua::internals::ValueTypes::INVOCABLE>(register_usage_profile,
                                                                                    *r);
                }
            } else if (opcode == DEFER) {
                auto fn = instruction->operands.at(0).get();
                if ((not dynamic_cast<AtomLiteral*>(fn)) and (not dynamic_cast<FunctionNameLiteral*>(fn)) and
                    (not dynamic_cast<RegisterIndex*>(fn))) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected function name, atom literal, or register index");
                }
                if (auto r = dynamic_cast<RegisterIndex*>(fn); r) {
                    check_use_of_register(register_usage_profile, *r);
                    assert_type_of_register<viua::internals::ValueTypes::INVOCABLE>(register_usage_profile,
                                                                                    *r);
                }
            } else if (opcode == ARG) {
                if (get_operand<VoidLiteral>(*instruction, 0)) {
                    continue;
                }

                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index or void");
                }

                check_if_name_resolved(register_usage_profile, *target);

                auto val = Register(*target);
                register_usage_profile.define(val, target->tokens.at(0));
            } else if (opcode == ARGC) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index or void");
                }

                check_if_name_resolved(register_usage_profile, *target);

                auto val = Register(*target);
                val.value_type = ValueTypes::INTEGER;
                register_usage_profile.define(val, target->tokens.at(0));
            } else if (opcode == PROCESS) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    if (not get_operand<VoidLiteral>(*instruction, 0)) {
                        throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                            .note("expected register index or void");
                    }
                }

                if (target) {
                    check_if_name_resolved(register_usage_profile, *target);
                }

                auto fn = instruction->operands.at(1).get();
                if ((not dynamic_cast<AtomLiteral*>(fn)) and (not dynamic_cast<FunctionNameLiteral*>(fn)) and
                    (not dynamic_cast<RegisterIndex*>(fn))) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected function name, atom literal, or register index");
                }
                if (auto r = dynamic_cast<RegisterIndex*>(fn); r) {
                    check_use_of_register(register_usage_profile, *r);
                    assert_type_of_register<viua::internals::ValueTypes::INVOCABLE>(register_usage_profile,
                                                                                    *r);
                }

                if (target) {
                    auto val = Register{*target};
                    val.value_type = ValueTypes::PID;
                    register_usage_profile.define(val, target->tokens.at(0));
                }
            } else if (opcode == SELF) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    if (not get_operand<VoidLiteral>(*instruction, 0)) {
                        throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                            .note("expected register index or void");
                    }
                }

                check_if_name_resolved(register_usage_profile, *target);

                auto val = Register{*target};
                val.value_type = ValueTypes::PID;
                register_usage_profile.define(val, target->tokens.at(0));
            } else if (opcode == JOIN) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *target);
                if (target->as != viua::internals::AccessSpecifier::DIRECT) {
                    throw InvalidSyntax(target->tokens.at(0), "invalid access mode")
                        .note("can only delete using direct access mode")
                        .aside("did you mean '%" + target->tokens.at(0).str().substr(1) + "'?");
                }

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::PID>(register_usage_profile, *source);

                auto val = Register{*target};
                register_usage_profile.define(val, target->tokens.at(0));
            } else if (opcode == SEND) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *target, "send target from");
                assert_type_of_register<viua::internals::ValueTypes::PID>(register_usage_profile, *target);

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source, "send from");
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *target);
                erase_if_direct_access(register_usage_profile, source, instruction);
            } else if (opcode == RECEIVE) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    if (not get_operand<VoidLiteral>(*instruction, 0)) {
                        throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                            .note("expected register index or void");
                    }
                }

                if (target) {
                    check_if_name_resolved(register_usage_profile, *target);

                    auto val = Register{*target};
                    register_usage_profile.define(val, target->tokens.at(0));
                }
            } else if (opcode == WATCHDOG) {
                auto fn = instruction->operands.at(0).get();
                if ((not dynamic_cast<AtomLiteral*>(fn)) and (not dynamic_cast<FunctionNameLiteral*>(fn))) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected function name or atom literal");
                }
            } else if (opcode == JUMP) {
                auto target = instruction->operands.at(0).get();

                if (auto offset = dynamic_cast<Offset*>(target); offset) {
                    auto jump_target = (stol(offset->tokens.at(0).str().substr(1)) - 1);
                    if (jump_target > 0) {
                        // FIXME use a recursive call and an immediate return instead of messing around with
                        // loop variable
                        i += static_cast<decltype(i)>(jump_target);
                    }
                    continue;
                } else if (auto label = dynamic_cast<Label*>(target); label) {
                    auto jump_target = ib.marker_map.at(label->tokens.at(0));
                    if (jump_target > i) {
                        // FIXME use a recursive call and an immediate return instead of messing around with
                        // loop variable
                        i = jump_target;
                    }
                } else if (str::ishex(target->tokens.at(0))) {
                    // FIXME Disassembler outputs '0x...' hexadecimal targets for if and jump instructions.
                    // Do not check them now, but this should be fixed in the future.
                    return;
                } else {
                    throw InvalidSyntax(target->tokens.at(0), "invalid operand for jump instruction");
                }
            } else if (opcode == IF) {
                auto source = get_operand<RegisterIndex>(*instruction, 0);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }
                check_use_of_register(register_usage_profile, *source, "branch depends on");
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *source);

                auto jump_target_if_true = InstructionIndex{0};
                if (auto offset = get_operand<Offset>(*instruction, 1); offset) {
                    auto jump_target = stol(offset->tokens.at(0).str().substr(1));
                    if (jump_target > 0) {
                        jump_target_if_true = get_line_index_of_instruction(
                            mnemonic_counter + static_cast<decltype(i)>(jump_target), ib);
                    } else {
                        // XXX FIXME Checking backward jumps is tricky, beware of loops.
                        continue;
                    }
                } else if (auto label = get_operand<Label>(*instruction, 1); label) {
                    auto jump_target =
                        get_line_index_of_instruction(ib.marker_map.at(label->tokens.at(0)), ib);
                    if (jump_target > i) {
                        jump_target_if_true = jump_target;
                    } else {
                        // XXX FIXME Checking backward jumps is tricky, beware of loops.
                        continue;
                    }
                } else if (str::ishex(instruction->operands.at(1)->tokens.at(0))) {
                    // FIXME Disassembler outputs '0x...' hexadecimal targets for if and jump instructions.
                    // Do not check them now, but this should be fixed in the future.
                    continue;
                } else {
                    throw InvalidSyntax(instruction->operands.at(1)->tokens.at(0),
                                        "invalid operand for jump instruction");
                }

                auto jump_target_if_false = InstructionIndex{0};
                if (auto offset = get_operand<Offset>(*instruction, 2); offset) {
                    auto jump_target = stol(offset->tokens.at(0).str().substr(1));
                    if (jump_target > 0) {
                        jump_target_if_false = get_line_index_of_instruction(
                            mnemonic_counter + static_cast<decltype(i)>(jump_target), ib);
                    } else {
                        // XXX FIXME Checking backward jumps is tricky, beware of loops.
                        continue;
                    }
                } else if (auto label = get_operand<Label>(*instruction, 2); label) {
                    auto jump_target =
                        get_line_index_of_instruction(ib.marker_map.at(label->tokens.at(0)), ib);
                    if (jump_target > i) {
                        jump_target_if_false = jump_target;
                    } else {
                        // XXX FIXME Checking backward jumps is tricky, beware of loops.
                        continue;
                    }
                } else if (str::ishex(instruction->operands.at(2)->tokens.at(0))) {
                    // FIXME Disassembler outputs '0x...' hexadecimal targets for if and jump instructions.
                    // Do not check them now, but this should be fixed in the future.
                    continue;
                } else {
                    throw InvalidSyntax(instruction->operands.at(2)->tokens.at(0),
                                        "invalid operand for jump instruction");
                }

                string register_with_unused_value;

                try {
                    RegisterUsageProfile register_usage_profile_if_true = register_usage_profile;
                    check_register_usage_for_instruction_block_impl(register_usage_profile_if_true, ps, ib,
                                                                    jump_target_if_true, mnemonic_counter);
                } catch (viua::cg::lex::UnusedValue& e) {
                    // Do not fail yet, because the value may be used by false branch.
                    // Save the error for later rethrowing.
                    register_with_unused_value = e.what();
                } catch (InvalidSyntax& e) {
                    throw TracedSyntaxError{}.append(e).append(
                        InvalidSyntax{instruction->tokens.at(0), "after taking true branch here:"}.add(
                            instruction->operands.at(1)->tokens.at(0)));
                } catch (TracedSyntaxError& e) {
                    throw e.append(
                        InvalidSyntax{instruction->tokens.at(0), "after taking true branch here:"}.add(
                            instruction->operands.at(1)->tokens.at(0)));
                }

                try {
                    RegisterUsageProfile register_usage_profile_if_false = register_usage_profile;
                    check_register_usage_for_instruction_block_impl(register_usage_profile_if_false, ps, ib,
                                                                    jump_target_if_false, mnemonic_counter);
                } catch (viua::cg::lex::UnusedValue& e) {
                    if (register_with_unused_value == e.what()) {
                        throw TracedSyntaxError{}.append(e).append(
                            InvalidSyntax{instruction->tokens.at(0), "after taking either branch:"});
                    } else {
                        /*
                         * If an error was throw for a different register it means that the register that
                         * was unused in true branch was used in the false one (so no errror), and
                         * the register for which the false branch threw was used in the true one (so no
                         * error either).
                         */
                    }
                } catch (InvalidSyntax& e) {
                    throw TracedSyntaxError{}.append(e).append(
                        InvalidSyntax{instruction->tokens.at(0), "after taking false branch here:"}.add(
                            instruction->operands.at(2)->tokens.at(0)));
                } catch (TracedSyntaxError& e) {
                    throw e.append(
                        InvalidSyntax{instruction->tokens.at(0), "after taking false branch here:"}.add(
                            instruction->operands.at(2)->tokens.at(0)));
                }

                return;
            } else if (opcode == THROW) {
                auto source = get_operand<RegisterIndex>(*instruction, 0);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source, "throw from");
                assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(register_usage_profile,
                                                                                *source);
                erase_if_direct_access(register_usage_profile, source, instruction);

                /*
                 * If we reached a throw instruction there is no reason to continue analysing instructions, as
                 * at runtime the execution of the function would stop.
                 */
                try {
                    check_for_unused_registers(register_usage_profile);
                    check_closure_instantiations(register_usage_profile, ps, created_closures);
                } catch (InvalidSyntax& e) {
                    throw TracedSyntaxError{}.append(e).append(
                        InvalidSyntax{instruction->tokens.at(0), "after a throw here:"});
                } catch (TracedSyntaxError& e) {
                    throw e.append(InvalidSyntax{instruction->tokens.at(0), "after a throw here:"});
                }

                return;
            } else if (opcode == CATCH) {
                // FIXME TODO SA for entered blocks
            } else if (opcode == DRAW) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                register_usage_profile.define(Register{*target}, target->tokens.at(0));
            } else if (opcode == TRY) {
                // do nothing
            } else if (opcode == ENTER) {
                // FIXME TODO SA for entered blocks
            } else if (opcode == LEAVE) {
                // do nothing
            } else if (opcode == IMPORT) {
                // do nothing
            } else if (opcode == CLASS) {
                // TODO
            } else if (opcode == DERIVE) {
                // TODO
            } else if (opcode == ATTACH) {
                // TODO
            } else if (opcode == REGISTER) {
                // TODO
            } else if (opcode == ATOM) {
                auto operand = get_operand<RegisterIndex>(*instruction, 0);
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *operand);

                auto source = get_operand<AtomLiteral>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected atom literal");
                }

                auto val = Register{};
                val.index = operand->index;
                val.register_set = operand->rss;
                val.value_type = ValueTypes::ATOM;

                register_usage_profile.define(val, operand->tokens.at(0));
            } else if (opcode == ATOMEQ) {
                auto result = get_operand<RegisterIndex>(*instruction, 0);
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *result);

                auto lhs = get_operand<RegisterIndex>(*instruction, 1);
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens,
                                         "invalid left-hand side operand")
                        .note("expected register index");
                }

                auto rhs = get_operand<RegisterIndex>(*instruction, 2);
                if (not rhs) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens,
                                         "invalid right-hand side operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *lhs);
                check_use_of_register(register_usage_profile, *rhs);

                assert_type_of_register<ValueTypes::ATOM>(register_usage_profile, *lhs);
                assert_type_of_register<ValueTypes::ATOM>(register_usage_profile, *rhs);

                auto val = Register(*result);
                val.value_type = viua::internals::ValueTypes::BOOLEAN;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == STRUCT) {
                auto operand = get_operand<RegisterIndex>(*instruction, 0);
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *operand);

                auto val = Register{*operand};
                val.value_type = ValueTypes::STRUCT;
                register_usage_profile.define(val, operand->tokens.at(0));
            } else if (opcode == STRUCTINSERT) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *target);
                assert_type_of_register<viua::internals::ValueTypes::STRUCT>(register_usage_profile, *target);

                auto key = get_operand<RegisterIndex>(*instruction, 1);
                if (not key) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *key);
                assert_type_of_register<viua::internals::ValueTypes::ATOM>(register_usage_profile, *key);

                auto source = get_operand<RegisterIndex>(*instruction, 2);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                erase_if_direct_access(register_usage_profile, source, instruction);
            } else if (opcode == STRUCTREMOVE) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    if (not get_operand<VoidLiteral>(*instruction, 0)) {
                        throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                            .note("expected register index or void literal");
                    }
                }

                if (target) {
                    check_use_of_register(register_usage_profile, *target);
                }

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::STRUCT>(register_usage_profile, *source);

                auto key = get_operand<RegisterIndex>(*instruction, 2);
                if (not key) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *key);
                assert_type_of_register<viua::internals::ValueTypes::ATOM>(register_usage_profile, *key);

                if (target) {
                    register_usage_profile.define(Register{*target}, target->tokens.at(0));
                }
            } else if (opcode == STRUCTKEYS) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *target);

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::STRUCT>(register_usage_profile, *source);

                auto val = Register{*target};
                val.value_type = ValueTypes::VECTOR;
                register_usage_profile.define(val, target->tokens.at(0));
            } else if (opcode == NEW) {
                auto operand = get_operand<RegisterIndex>(*instruction, 0);
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_if_name_resolved(register_usage_profile, *operand);

                auto val = Register{};
                val.index = operand->index;
                val.register_set = operand->rss;
                val.value_type = viua::internals::ValueTypes::OBJECT;

                register_usage_profile.define(val, operand->tokens.at(0));
            } else if (opcode == MSG) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    if (not get_operand<VoidLiteral>(*instruction, 0)) {
                        throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                            .note("expected register index or void");
                    }
                }

                if (target) {
                    check_if_name_resolved(register_usage_profile, *target);
                }

                auto fn = instruction->operands.at(1).get();
                if ((not dynamic_cast<AtomLiteral*>(fn)) and (not dynamic_cast<FunctionNameLiteral*>(fn)) and
                    (not dynamic_cast<RegisterIndex*>(fn))) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected function name, atom literal, or register index");
                }
                if (auto r = dynamic_cast<RegisterIndex*>(fn); r) {
                    check_use_of_register(register_usage_profile, *r, "call from");
                    assert_type_of_register<viua::internals::ValueTypes::INVOCABLE>(register_usage_profile,
                                                                                    *r);
                }

                if (target) {
                    register_usage_profile.define(Register{*target}, target->tokens.at(0));
                }
            } else if (opcode == INSERT) {
                // TODO
            } else if (opcode == REMOVE) {
                auto target = get_operand<RegisterIndex>(*instruction, 0);
                if (not target) {
                    if (not get_operand<VoidLiteral>(*instruction, 0)) {
                        throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand")
                            .note("expected register index or void literal");
                    }
                }

                if (target) {
                    check_use_of_register(register_usage_profile, *target);
                }

                auto source = get_operand<RegisterIndex>(*instruction, 1);
                if (not source) {
                    throw invalid_syntax(instruction->operands.at(1)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *source);
                assert_type_of_register<viua::internals::ValueTypes::OBJECT>(register_usage_profile, *source);

                auto key = get_operand<RegisterIndex>(*instruction, 2);
                if (not key) {
                    throw invalid_syntax(instruction->operands.at(2)->tokens, "invalid operand")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *key);
                assert_type_of_register<viua::internals::ValueTypes::STRING>(register_usage_profile, *key);

                if (target) {
                    register_usage_profile.define(Register{*target}, target->tokens.at(0));
                }
            } else if (opcode == RETURN) {
                // do nothing
            } else if (opcode == HALT) {
                // do nothing
            }
        } catch (InvalidSyntax& e) { throw e.add(instruction->tokens.at(0)); } catch (TracedSyntaxError& e) {
            e.errors.at(0).add(instruction->tokens.at(0));
            throw e;
        }
    }

    /*
     * These checks should only be run if the SA actually reached the end of the instructions block.
     */
    check_for_unused_registers(register_usage_profile);
    check_closure_instantiations(register_usage_profile, ps, created_closures);
}
static auto check_register_usage_for_instruction_block(const ParsedSource& ps, const InstructionsBlock& ib)
    -> void {
    if (ib.closure) {
        return;
    }

    RegisterUsageProfile register_usage_profile;
    map_names_to_register_indexes(register_usage_profile, ib);
    check_register_usage_for_instruction_block_impl(register_usage_profile, ps, ib, 0,
                                                    static_cast<InstructionIndex>(-1));
}
auto viua::assembler::frontend::static_analyser::check_register_usage(const ParsedSource& src) -> void {
    verify_wrapper(src, check_register_usage_for_instruction_block);
}
