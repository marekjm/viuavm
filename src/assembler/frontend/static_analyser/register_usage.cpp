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
    auto defined(const Register r) const -> bool { return defined_registers.count(r); }
    auto defined_where(const Register r) const -> Token { return defined_registers.at(r).first; }

    auto define(const Register r, const Token t) -> void {
        defined_registers.emplace(r, pair(t, r.value_type));
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

static auto check_use_of_register(RegisterUsageProfile& rup,
                                  viua::assembler::frontend::parser::RegisterIndex r) {
    cerr << "check_use_of_register: " << r.index << endl;
    if (not rup.defined(Register(r))) {
        throw InvalidSyntax(r.tokens.at(0), ("use of empty register: " + to_string(r.index)));
    }
    rup.use(Register(r), r.tokens.at(0));
}

auto viua::assembler::frontend::static_analyser::check_register_usage(const ParsedSource& src) -> void {
    verify_wrapper(src, [](const ParsedSource&, const InstructionsBlock& ib) -> void {
        RegisterUsageProfile register_usage_profile;

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
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand for 'istore'")
                        .note("expected register index");
                }

                auto val = Register{};
                val.index = operand->index;
                val.register_set = operand->rss;
                val.value_type = viua::internals::ValueTypes::INTEGER;
                register_usage_profile.define(val, operand->tokens.at(0));
            } else if (opcode == SUB) {
                auto result = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not result) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand for 'sub'")
                        .note("expected register index");
                }

                auto lhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(1).get());
                if (not lhs) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand for 'sub'")
                        .note("expected register index");
                }

                auto rhs = dynamic_cast<RegisterIndex*>(instruction->operands.at(2).get());
                if (not rhs) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand for 'sub'")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *rhs);
                check_use_of_register(register_usage_profile, *lhs);

                if (register_usage_profile.at(Register(*lhs)).second !=
                    viua::internals::ValueTypes::INTEGER) {
                    throw TracedSyntaxError{}
                        .append(
                            InvalidSyntax(lhs->tokens.at(0), "invalid type of value contained in register")
                                .note("expected integer"))
                        .append(InvalidSyntax(register_usage_profile.defined_where(Register(*lhs)), "")
                                    .note("defined here"));
                }
                if (register_usage_profile.at(Register(*rhs)).second !=
                    viua::internals::ValueTypes::INTEGER) {
                    throw TracedSyntaxError{}
                        .append(
                            InvalidSyntax(rhs->tokens.at(0), "invalid type of value contained in register")
                                .note("expected integer"))
                        .append(InvalidSyntax(register_usage_profile.defined_where(Register(*rhs)), "")
                                    .note("defined here"));
                }

                auto val = Register(*result);
                val.value_type = register_usage_profile.at(*lhs).second;
                register_usage_profile.define(val, result->tokens.at(0));
            } else if (opcode == TEXT) {
                auto operand = dynamic_cast<RegisterIndex*>(instruction->operands.at(0).get());
                if (not operand) {
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand for 'text'")
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
                    throw invalid_syntax(instruction->operands.at(0)->tokens, "invalid operand for 'print'")
                        .note("expected register index");
                }

                check_use_of_register(register_usage_profile, *operand);

                register_usage_profile.use(Register(*operand), operand->tokens.at(0));
            }
        }

        for (const auto& each : register_usage_profile) {
            if (not register_usage_profile.used(each.first)) {
                throw InvalidSyntax(each.second.first,
                                    ("unused value in register " + to_string(each.first.index)));
            }
        }
    });
}
