/*
 *  Copyright (C) 2018 Marek Marecki
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

#include <string>
#include <vector>
#include <viua/assembler/frontend/static_analyser.h>
#include <viua/support/string.h>

using viua::assembler::frontend::parser::Instruction;

namespace viua {
    namespace assembler {
        namespace frontend {
            namespace static_analyser {
                namespace checkers {
                    using viua::assembler::frontend::parser::AtomLiteral;
                    using viua::assembler::frontend::parser::BitsLiteral;
                    using viua::assembler::frontend::parser::BooleanLiteral;
                    using viua::assembler::frontend::parser::FunctionNameLiteral;
                    using viua::assembler::frontend::parser::Label;
                    using viua::assembler::frontend::parser::Offset;
                    using viua::assembler::frontend::parser::RegisterIndex;
                    using viua::assembler::frontend::parser::VoidLiteral;
                    using viua::cg::lex::InvalidSyntax;
                    using viua::cg::lex::Token;
                    using viua::cg::lex::TracedSyntaxError;

                    using viua::internals::RegisterSets;
                    auto register_set_names = std::map<RegisterSets, std::string>{
                        {
                            // FIXME 'current' as a register set name should be deprecated.
                            RegisterSets::CURRENT,
                            "current",
                        },
                        {
                            RegisterSets::GLOBAL,
                            "global",
                        },
                        {
                            RegisterSets::STATIC,
                            "static",
                        },
                        {
                            RegisterSets::LOCAL,
                            "local",
                        },
                    };
                    auto to_string(RegisterSets const register_set_id) -> std::string {
                        return register_set_names.at(register_set_id);
                    }

                    auto invalid_syntax(std::vector<Token> const& tokens, std::string const message)
                        -> InvalidSyntax {
                        auto invalid_syntax_error = InvalidSyntax(tokens.at(0), message);
                        for (auto i = std::remove_reference_t<decltype(tokens)>::size_type{1};
                             i < tokens.size(); ++i) {
                            invalid_syntax_error.add(tokens.at(i));
                        }
                        return invalid_syntax_error;
                    }

                    auto get_line_index_of_instruction(InstructionIndex const n, InstructionsBlock const& ib) -> InstructionIndex {
                        auto left = n;
                        auto i = InstructionIndex{0};
                        for (; left and i < ib.body.size(); ++i) {
                            auto instruction = dynamic_cast<viua::assembler::frontend::parser::Instruction*>(
                                ib.body.at(i).get());
                            if (instruction) {
                                --left;
                            }
                        }
                        return i;
                    }

                    auto erase_if_direct_access( Register_usage_profile& register_usage_profile, RegisterIndex* const r, viua::assembler::frontend::parser::Instruction const& instruction) -> void {
                        if (r->as == viua::internals::AccessSpecifier::DIRECT) {
                            register_usage_profile.erase(Register(*r), instruction.tokens.at(0));
                        }
                    }

                    template<typename K, typename V>
                    static auto keys_of(std::map<K, V> const& m) -> std::vector<K> {
                        auto keys = std::vector<K>{};

                        for (const auto& each : m) {
                            keys.push_back(each.first);
                        }

                        return keys;
                    }
                    auto check_if_name_resolved(Register_usage_profile const& rup, RegisterIndex const r)
                        -> void {
                        if (not r.resolved) {
                            auto error = InvalidSyntax(r.tokens.at(0), "unresolved name");
                            if (auto suggestion = str::levenshtein_best(r.tokens.at(0).str().substr(1),
                                                                        keys_of(rup.name_to_index), 4);
                                suggestion.first) {
                                error.aside(r.tokens.at(0),
                                            "did you mean '" + suggestion.second + "' (name of " +
                                                std::to_string(rup.name_to_index.at(suggestion.second)) +
                                                ")?");
                            }
                            throw error;
                        }
                    }
                    static auto maybe_mistyped_register_set_helper(
                        Register_usage_profile& rup, viua::assembler::frontend::parser::RegisterIndex r,
                        TracedSyntaxError& error, RegisterSets rs_id) -> bool {
                        if (r.rss != rs_id) {
                            auto val = Register{};
                            val.index = r.index;
                            val.register_set = rs_id;
                            if (rup.defined(val)) {
                                error.errors.back().aside(r.tokens.at(0), "did you mean " + to_string(rs_id) +
                                                                              " register " +
                                                                              std::to_string(r.index) + "?");
                                error.append(InvalidSyntax(rup.defined_where(val), "")
                                                 .note(to_string(rs_id) + " register " +
                                                       std::to_string(r.index) + " was defined here"));
                                return true;
                            }
                        }
                        return false;
                    }
                    static auto maybe_mistyped_register_set(
                        Register_usage_profile& rup, viua::assembler::frontend::parser::RegisterIndex r,
                        TracedSyntaxError& error) -> void {
                        if (maybe_mistyped_register_set_helper(rup, r, error, RegisterSets::LOCAL)) {
                            return;
                        }
                        if (maybe_mistyped_register_set_helper(rup, r, error, RegisterSets::STATIC)) {
                            return;
                        }
                    }
                    auto check_use_of_register(Register_usage_profile& rup,
                                               viua::assembler::frontend::parser::RegisterIndex r,
                                               std::string const error_core_msg) -> void {
                        check_if_name_resolved(rup, r);
                        if (not rup.defined(Register(r))) {
                            auto empty_or_erased = (rup.erased(Register(r)) ? "erased" : "empty");

                            auto msg = std::ostringstream{};
                            msg << error_core_msg << ' ' << empty_or_erased << ' ' << to_string(r.rss)
                                << " register " << str::enquote(std::to_string(r.index));
                            if (rup.index_to_name.count(r.index)) {
                                msg << " (named " << str::enquote(rup.index_to_name.at(r.index)) << ')';
                            }
                            auto error = TracedSyntaxError{}.append(InvalidSyntax(r.tokens.at(0), msg.str()));

                            if (rup.erased(Register(r))) {
                                error.append(
                                    InvalidSyntax(rup.erased_where(Register(r)), "").note("erased here:"));
                            }

                            maybe_mistyped_register_set(rup, r, error);

                            throw error;
                        }
                        rup.use(Register(r), r.tokens.at(0));
                    }

                    using ValueTypes = viua::internals::ValueTypes;
                    using ValueTypesType = viua::internals::ValueTypesType;
                    auto const value_type_names = std::map<ValueTypes, std::string>{
                        {
                            /*
                             * Making this "value" instead of "undefined" lets us build error messages for
                             * unused registers. They "unused value" can become "unused number" or "unused
                             * boolean", and if the SA could not infer the type for the value we will not
                             * print a "unused undefined" (what would that even mean?) message, but "unused
                             * value".
                             */
                            ValueTypes::UNDEFINED,
                            "value",
                        },
                        {
                            ValueTypes::VOID,
                            "void",
                        },
                        {
                            ValueTypes::INTEGER,
                            "integer",
                        },
                        {
                            ValueTypes::FLOAT,
                            "float",
                        },
                        {
                            ValueTypes::NUMBER,
                            "number",
                        },
                        {
                            ValueTypes::BOOLEAN,
                            "boolean",
                        },
                        {
                            ValueTypes::TEXT,
                            "text",
                        },
                        {
                            ValueTypes::STRING,
                            "string",
                        },
                        {
                            ValueTypes::VECTOR,
                            "vector",
                        },
                        {
                            ValueTypes::BITS,
                            "bits",
                        },
                        {
                            ValueTypes::CLOSURE,
                            "closure",
                        },
                        {
                            ValueTypes::FUNCTION,
                            "function",
                        },
                        {
                            ValueTypes::INVOCABLE,
                            "invocable",
                        },
                        {
                            ValueTypes::ATOM,
                            "atom",
                        },
                        {
                            ValueTypes::PID,
                            "pid",
                        },
                        {
                            ValueTypes::STRUCT,
                            "struct",
                        },
                        {
                            ValueTypes::OBJECT,
                            "object",
                        },
                    };
                    auto operator|(const ValueTypes lhs, const ValueTypes rhs) -> ValueTypes {
                        // FIXME find out if it is possible to remove the outermost static_cast<>
                        return static_cast<ValueTypes>(static_cast<ValueTypesType>(lhs) |
                                                       static_cast<ValueTypesType>(rhs));
                    }
                    auto operator&(const ValueTypes lhs, const ValueTypes rhs) -> ValueTypes {
                        // FIXME find out if it is possible to remove the outermost static_cast<>
                        return static_cast<ValueTypes>(static_cast<ValueTypesType>(lhs) &
                                                       static_cast<ValueTypesType>(rhs));
                    }
                    auto operator^(const ValueTypes lhs, const ValueTypes rhs) -> ValueTypes {
                        // FIXME find out if it is possible to remove the outermost static_cast<>
                        return static_cast<ValueTypes>(static_cast<ValueTypesType>(lhs) ^
                                                       static_cast<ValueTypesType>(rhs));
                    }
                    auto operator!(const ValueTypes v) -> bool { return not static_cast<ValueTypesType>(v); }
                    auto to_string(ValueTypes const value_type_id) -> std::string {
                        auto const has_pointer = not not(value_type_id & ValueTypes::POINTER);
                        auto const type_name = value_type_names.at(
                            has_pointer ? (value_type_id ^ ValueTypes::POINTER) : value_type_id);
                        return (has_pointer ? "pointer to " : "") + type_name;
                    }
                    auto depointerise_type_if_needed(ValueTypes const t,
                                                     bool const access_via_pointer_dereference)
                        -> ValueTypes {
                        return (access_via_pointer_dereference ? (t ^ ValueTypes::POINTER) : t);
                    }

                    auto check_op_izero(Register_usage_profile& register_usage_profile,
                                        Instruction const& instruction) -> void {
                        auto operand = get_operand<RegisterIndex>(instruction, 0);
                        if (not operand) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_if_name_resolved(register_usage_profile, *operand);

                        auto val = Register{};
                        val.index = operand->index;
                        val.register_set = operand->rss;
                        val.value_type = viua::internals::ValueTypes::INTEGER;
                        register_usage_profile.define(val, operand->tokens.at(0));
                    }
                    auto check_op_bit_rotates(Register_usage_profile& register_usage_profile,
                                              Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        auto offset = get_operand<RegisterIndex>(instruction, 1);
                        if (not offset) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *target);
                        check_use_of_register(register_usage_profile, *offset);

                        assert_type_of_register<viua::internals::ValueTypes::BITS>(register_usage_profile,
                                                                                   *target);
                        assert_type_of_register<viua::internals::ValueTypes::INTEGER>(register_usage_profile,
                                                                                      *offset);
                    }
                    auto check_op_copy(Register_usage_profile& register_usage_profile,
                                       Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        auto source = get_operand<RegisterIndex>(instruction, 1);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source, "copy from");
                        auto type_of_source = assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(
                            register_usage_profile, *source);

                        auto val = Register(*target);
                        val.value_type = type_of_source;
                        register_usage_profile.define(val, target->tokens.at(0));
                    }
                    auto check_op_move(Register_usage_profile& register_usage_profile,
                                       Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        auto source = get_operand<RegisterIndex>(instruction, 1);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source, "move from");
                        assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(
                            register_usage_profile, *source);

                        auto val = Register(*target);
                        val.value_type = register_usage_profile.at(*source).second.value_type;
                        register_usage_profile.define(val, target->tokens.at(0));

                        erase_if_direct_access(register_usage_profile, source, instruction);
                    }
                    auto check_op_ptr(Register_usage_profile& register_usage_profile,
                                      Instruction const& instruction) -> void {
                        auto result = get_operand<RegisterIndex>(instruction, 0);
                        if (not result) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_if_name_resolved(register_usage_profile, *result);

                        auto operand = get_operand<RegisterIndex>(instruction, 1);
                        if (not operand) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *operand, "pointer from");

                        auto val = Register(*result);
                        val.value_type = (register_usage_profile.at(Register(*operand)).second.value_type |
                                          viua::internals::ValueTypes::POINTER);
                        register_usage_profile.define(val, result->tokens.at(0));
                    }
                    auto check_op_capture(Register_usage_profile& register_usage_profile,
                                          Instruction const& instruction,
                                          std::map<Register, Closure>& created_closures) -> void {
                        auto closure = get_operand<RegisterIndex>(instruction, 0);
                        if (not closure) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *closure);
                        assert_type_of_register<viua::internals::ValueTypes::CLOSURE>(register_usage_profile,
                                                                                      *closure);

                        // this index is not verified because it is used as *the* index to use when
                        // putting a value inside the closure
                        auto index = get_operand<RegisterIndex>(instruction, 1);
                        if (not index) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        auto source = get_operand<RegisterIndex>(instruction, 2);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(2)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source);
                        assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(
                            register_usage_profile, *source);

                        auto val = Register{};
                        val.index = index->index;
                        val.register_set = RegisterSets::LOCAL;
                        val.value_type = register_usage_profile.at(*source).second.value_type;
                        created_closures.at(Register{*closure}).define(val, index->tokens.at(0));
                    }
                    auto check_op_capturecopy(Register_usage_profile& register_usage_profile,
                                              Instruction const& instruction,
                                              std::map<Register, Closure>& created_closures) -> void {
                        auto closure = get_operand<RegisterIndex>(instruction, 0);
                        if (not closure) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *closure);
                        assert_type_of_register<viua::internals::ValueTypes::CLOSURE>(register_usage_profile,
                                                                                      *closure);

                        // this index is not verified because it is used as *the* index to use when
                        // putting a value inside the closure
                        auto index = get_operand<RegisterIndex>(instruction, 1);
                        if (not index) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        auto source = get_operand<RegisterIndex>(instruction, 2);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(2)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source);
                        assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(
                            register_usage_profile, *source);

                        auto val = Register{};
                        val.index = index->index;
                        val.register_set = RegisterSets::LOCAL;
                        val.value_type = register_usage_profile.at(*source).second.value_type;
                        created_closures.at(Register{*closure}).define(val, index->tokens.at(0));
                    }
                    auto check_op_capturemove(Register_usage_profile& register_usage_profile,
                                              Instruction const& instruction,
                                              std::map<Register, Closure>& created_closures) -> void {
                        auto closure = get_operand<RegisterIndex>(instruction, 0);
                        if (not closure) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *closure);
                        assert_type_of_register<viua::internals::ValueTypes::CLOSURE>(register_usage_profile,
                                                                                      *closure);

                        // this index is not verified because it is used as *the* index to use when
                        // putting a value inside the closure
                        auto index = get_operand<RegisterIndex>(instruction, 1);
                        if (not index) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        auto source = get_operand<RegisterIndex>(instruction, 2);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(2)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source);
                        assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(
                            register_usage_profile, *source);

                        auto val = Register{};
                        val.index = index->index;
                        val.register_set = RegisterSets::LOCAL;
                        val.value_type = register_usage_profile.at(*source).second.value_type;
                        created_closures.at(Register{*closure}).define(val, index->tokens.at(0));

                        erase_if_direct_access(register_usage_profile, source, instruction);
                    }
                    auto check_op_closure(Register_usage_profile& register_usage_profile,
                                          Instruction const& instruction,
                                          std::map<Register, Closure>& created_closures) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_if_name_resolved(register_usage_profile, *target);

                        auto fn = get_operand<FunctionNameLiteral>(instruction, 1);
                        if (not fn) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
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
                    }
                    auto check_op_function(Register_usage_profile& register_usage_profile,
                                           Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_if_name_resolved(register_usage_profile, *target);

                        if (not get_operand<FunctionNameLiteral>(instruction, 1)) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected function name literal");
                        }

                        auto val = Register{*target};
                        val.value_type = ValueTypes::FUNCTION;
                        register_usage_profile.define(val, target->tokens.at(0));
                    }
                    auto check_op_frame(Register_usage_profile&, Instruction const&) -> void {
                        // do nothing, FRAMEs do not modify registers;
                        // also, frame balance and arity is handled by verifier which runs before SA
                    }
                    auto check_op_param(Register_usage_profile& register_usage_profile,
                                        Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        if (target->as == viua::internals::AccessSpecifier::REGISTER_INDIRECT) {
                            auto r = *target;
                            r.rss = viua::internals::RegisterSets::LOCAL;
                            check_use_of_register(register_usage_profile, r);
                            assert_type_of_register<viua::internals::ValueTypes::INTEGER>(
                                register_usage_profile, r);
                        }

                        auto source = get_operand<RegisterIndex>(instruction, 1);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source);
                        assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(
                            register_usage_profile, *source);
                    }
                    auto check_op_pamv(Register_usage_profile& register_usage_profile,
                                       Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }
                        if (target->as == viua::internals::AccessSpecifier::REGISTER_INDIRECT) {
                            auto r = *target;
                            r.rss = viua::internals::RegisterSets::LOCAL;
                            check_use_of_register(register_usage_profile, r);
                            assert_type_of_register<viua::internals::ValueTypes::INTEGER>(
                                register_usage_profile, r);
                        }

                        auto source = get_operand<RegisterIndex>(instruction, 1);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source);
                        assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(
                            register_usage_profile, *source);

                        erase_if_direct_access(register_usage_profile, source, instruction);
                    }
                    auto check_op_defer(Register_usage_profile& register_usage_profile,
                                        Instruction const& instruction) -> void {
                        auto fn = instruction.operands.at(0).get();
                        if ((not dynamic_cast<AtomLiteral*>(fn)) and
                            (not dynamic_cast<FunctionNameLiteral*>(fn)) and
                            (not dynamic_cast<RegisterIndex*>(fn))) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected function name, atom literal, or register index");
                        }
                        if (auto r = dynamic_cast<RegisterIndex*>(fn); r) {
                            check_use_of_register(register_usage_profile, *r);
                            assert_type_of_register<viua::internals::ValueTypes::INVOCABLE>(
                                register_usage_profile, *r);
                        }
                    }
                    auto check_op_process(Register_usage_profile& register_usage_profile,
                                          Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            if (not get_operand<VoidLiteral>(instruction, 0)) {
                                throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                    .note("expected register index or void");
                            }
                        }

                        if (target) {
                            check_if_name_resolved(register_usage_profile, *target);
                        }

                        auto fn = instruction.operands.at(1).get();
                        if ((not dynamic_cast<AtomLiteral*>(fn)) and
                            (not dynamic_cast<FunctionNameLiteral*>(fn)) and
                            (not dynamic_cast<RegisterIndex*>(fn))) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected function name, atom literal, or register index");
                        }
                        if (auto r = dynamic_cast<RegisterIndex*>(fn); r) {
                            check_use_of_register(register_usage_profile, *r);
                            assert_type_of_register<viua::internals::ValueTypes::INVOCABLE>(
                                register_usage_profile, *r);
                        }

                        if (target) {
                            auto val = Register{*target};
                            val.value_type = ValueTypes::PID;
                            register_usage_profile.define(val, target->tokens.at(0));
                        }
                    }
                    auto check_op_self(Register_usage_profile& register_usage_profile,
                                       Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            if (not get_operand<VoidLiteral>(instruction, 0)) {
                                throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                    .note("expected register index or void");
                            }
                        }

                        check_if_name_resolved(register_usage_profile, *target);

                        auto val = Register{*target};
                        val.value_type = ValueTypes::PID;
                        register_usage_profile.define(val, target->tokens.at(0));
                    }
                    auto check_op_join(Register_usage_profile& register_usage_profile,
                                       Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            if (not get_operand<VoidLiteral>(instruction, 0)) {
                                throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                    .note("expected register index or void");
                            }
                        }

                        if (target) {
                            check_if_name_resolved(register_usage_profile, *target);
                            if (target->as != viua::internals::AccessSpecifier::DIRECT) {
                                throw InvalidSyntax(target->tokens.at(0), "invalid access mode")
                                    .note("can only join using direct access mode")
                                    .aside("did you mean '%" + target->tokens.at(0).str().substr(1) + "'?");
                            }
                        }

                        auto source = get_operand<RegisterIndex>(instruction, 1);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source);
                        assert_type_of_register<viua::internals::ValueTypes::PID>(register_usage_profile,
                                                                                  *source);

                        if (target) {
                            auto val = Register{*target};
                            register_usage_profile.define(val, target->tokens.at(0));
                        }
                    }
                    auto check_op_send(Register_usage_profile& register_usage_profile,
                                       Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *target, "send target from");
                        assert_type_of_register<viua::internals::ValueTypes::PID>(register_usage_profile,
                                                                                  *target);

                        auto source = get_operand<RegisterIndex>(instruction, 1);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source, "send from");
                        assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(
                            register_usage_profile, *target);
                        erase_if_direct_access(register_usage_profile, source, instruction);
                    }
                    auto check_op_receive(Register_usage_profile& register_usage_profile,
                                          Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            if (not get_operand<VoidLiteral>(instruction, 0)) {
                                throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                    .note("expected register index or void");
                            }
                        }

                        if (target) {
                            check_if_name_resolved(register_usage_profile, *target);

                            auto val = Register{*target};
                            register_usage_profile.define(val, target->tokens.at(0));
                        }
                    }
                    auto check_op_draw(Register_usage_profile& register_usage_profile,
                                       Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        register_usage_profile.define(Register{*target}, target->tokens.at(0));
                    }
                    auto check_op_enter(Register_usage_profile& register_usage_profile,
                                        ParsedSource const& ps, Instruction const& instruction) -> void {
                        auto const label = get_operand<Label>(instruction, 0);
                        if (not label) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected a block name");
                        }

                        auto const block_name = label->tokens.at(0).str();

                        try {
                            check_register_usage_for_instruction_block_impl(register_usage_profile, ps,
                                                                            ps.block(block_name), 0, 0);
                        } catch (InvalidSyntax& e) {
                            throw TracedSyntaxError{}.append(e).append(
                                InvalidSyntax{label->tokens.at(0), "after entering block " + block_name});
                        } catch (TracedSyntaxError& e) {
                            throw e.append(
                                InvalidSyntax{label->tokens.at(0), "after entering block " + block_name});
                        }
                    }
                    auto check_op_jump(Register_usage_profile& register_usage_profile, ParsedSource const& ps,
                                       Instruction const& instruction, InstructionsBlock const& ib,
                                       InstructionIndex i, InstructionIndex const mnemonic_counter) -> void {
                        auto target = instruction.operands.at(0).get();

                        if (auto offset = dynamic_cast<Offset*>(target); offset) {
                            auto const jump_target = std::stol(offset->tokens.at(0));
                            if (jump_target > 0) {
                                i = get_line_index_of_instruction(
                                    mnemonic_counter + static_cast<decltype(i)>(jump_target), ib);
                                check_register_usage_for_instruction_block_impl(register_usage_profile, ps,
                                                                                ib, i, mnemonic_counter);
                            }
                        } else if (auto label = dynamic_cast<Label*>(target); label) {
                            auto jump_target = ib.marker_map.at(label->tokens.at(0));
                            jump_target = get_line_index_of_instruction(jump_target, ib);
                            if (jump_target > i) {
                                check_register_usage_for_instruction_block_impl(
                                    register_usage_profile, ps, ib, jump_target, mnemonic_counter);
                            }
                        } else if (str::ishex(target->tokens.at(0))) {
                            // FIXME Disassembler outputs '0x...' hexadecimal targets for if and jump
                            // instructions. Do not check them now, but this should be fixed in the future.
                        } else {
                            throw InvalidSyntax(target->tokens.at(0), "invalid operand for jump instruction");
                        }
                    }
                    auto check_op_if(Register_usage_profile& register_usage_profile, ParsedSource const& ps,
                                     Instruction const& instruction, InstructionsBlock const& ib,
                                     InstructionIndex i, InstructionIndex const mnemonic_counter) -> void {
                        auto source = get_operand<RegisterIndex>(instruction, 0);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }
                        check_use_of_register(register_usage_profile, *source, "branch depends on");
                        assert_type_of_register<viua::internals::ValueTypes::UNDEFINED>(
                            register_usage_profile, *source);

                        auto jump_target_if_true = InstructionIndex{0};
                        if (auto offset = get_operand<Offset>(instruction, 1); offset) {
                            auto const jump_target = std::stol(offset->tokens.at(0));
                            if (jump_target > 0) {
                                jump_target_if_true = get_line_index_of_instruction(
                                    mnemonic_counter + static_cast<decltype(i)>(jump_target), ib);
                            } else {
                                // XXX FIXME Checking backward jumps is tricky, beware of loops.
                                return;
                            }
                        } else if (auto label = get_operand<Label>(instruction, 1); label) {
                            auto jump_target =
                                get_line_index_of_instruction(ib.marker_map.at(label->tokens.at(0)), ib);
                            if (jump_target > i) {
                                jump_target_if_true = jump_target;
                            } else {
                                // XXX FIXME Checking backward jumps is tricky, beware of loops.
                                return;
                            }
                        } else if (str::ishex(instruction.operands.at(1)->tokens.at(0))) {
                            // FIXME Disassembler outputs '0x...' hexadecimal targets for if and jump
                            // instructions. Do not check them now, but this should be fixed in the future.
                            // FIXME Return now and abort further checking of this block or risk throwing
                            // *many* false positives.
                            return;
                        } else {
                            throw InvalidSyntax(instruction.operands.at(1)->tokens.at(0),
                                                "invalid operand for if instruction");
                        }

                        auto jump_target_if_false = InstructionIndex{0};
                        if (auto offset = get_operand<Offset>(instruction, 2); offset) {
                            auto const jump_target = std::stol(offset->tokens.at(0));
                            if (jump_target > 0) {
                                jump_target_if_false = get_line_index_of_instruction(
                                    mnemonic_counter + static_cast<decltype(i)>(jump_target), ib);
                            } else {
                                // XXX FIXME Checking backward jumps is tricky, beware of loops.
                                return;
                            }
                        } else if (auto label = get_operand<Label>(instruction, 2); label) {
                            auto jump_target =
                                get_line_index_of_instruction(ib.marker_map.at(label->tokens.at(0)), ib);
                            if (jump_target > i) {
                                jump_target_if_false = jump_target;
                            } else {
                                // XXX FIXME Checking backward jumps is tricky, beware of loops.
                                return;
                            }
                        } else if (str::ishex(instruction.operands.at(2)->tokens.at(0))) {
                            // FIXME Disassembler outputs '0x...' hexadecimal targets for if and jump
                            // instructions. Do not check them now, but this should be fixed in the future.
                            // FIXME Return now and abort further checking of this block or risk throwing
                            // *many* false positives.
                            return;
                        } else {
                            throw InvalidSyntax(instruction.operands.at(2)->tokens.at(0),
                                                "invalid operand for if instruction");
                        }

                        auto register_with_unused_value = std::string{};

                        try {
                            Register_usage_profile register_usage_profile_if_true = register_usage_profile;
                            register_usage_profile_if_true.defresh();
                            check_register_usage_for_instruction_block_impl(register_usage_profile_if_true,
                                                                            ps, ib, jump_target_if_true,
                                                                            mnemonic_counter);
                        } catch (viua::cg::lex::UnusedValue& e) {
                            // Do not fail yet, because the value may be used by false branch.
                            // Save the error for later rethrowing.
                            register_with_unused_value = e.what();
                        } catch (InvalidSyntax& e) {
                            throw TracedSyntaxError{}.append(e).append(
                                InvalidSyntax{instruction.tokens.at(0), "after taking true branch here:"}.add(
                                    instruction.operands.at(1)->tokens.at(0)));
                        } catch (TracedSyntaxError& e) {
                            throw e.append(
                                InvalidSyntax{instruction.tokens.at(0), "after taking true branch here:"}.add(
                                    instruction.operands.at(1)->tokens.at(0)));
                        }

                        try {
                            Register_usage_profile register_usage_profile_if_false = register_usage_profile;
                            register_usage_profile_if_false.defresh();
                            check_register_usage_for_instruction_block_impl(register_usage_profile_if_false,
                                                                            ps, ib, jump_target_if_false,
                                                                            mnemonic_counter);
                        } catch (viua::cg::lex::UnusedValue& e) {
                            if (register_with_unused_value == e.what()) {
                                throw TracedSyntaxError{}.append(e).append(
                                    InvalidSyntax{instruction.tokens.at(0), "after taking either branch:"});
                            } else {
                                /*
                                 * If an error was thrown for a different register it means that the register
                                 * that was unused in true branch was used in the false one (so no errror),
                                 * and the register for which the false branch threw was used in the true one
                                 * (so no error either).
                                 */
                            }
                        } catch (InvalidSyntax& e) {
                            if (register_with_unused_value != e.what() and
                                std::string{e.what()}.substr(0, 6) == "unused") {
                                /*
                                 * If an error was thrown for a different register it means that the register
                                 * that was unused in true branch was used in the false one (so no errror),
                                 * and the register for which the false branch threw was used in the true one
                                 * (so no error either).
                                 */
                            } else {
                                throw TracedSyntaxError{}.append(e).append(
                                    InvalidSyntax{instruction.tokens.at(0), "after taking false branch here:"}
                                        .add(instruction.operands.at(2)->tokens.at(0)));
                            }
                        } catch (TracedSyntaxError& e) {
                            if (register_with_unused_value != e.errors.front().what() and
                                std::string{e.errors.front().what()}.substr(0, 6) == "unused") {
                                /*
                                 * If an error was thrown for a different register it means that the register
                                 * that was unused in true branch was used in the false one (so no errror),
                                 * and the register for which the false branch threw was used in the true one
                                 * (so no error either).
                                 */
                            } else {
                                throw e.append(
                                    InvalidSyntax{instruction.tokens.at(0), "after taking false branch here:"}
                                        .add(instruction.operands.at(2)->tokens.at(0)));
                            }
                        }
                    }
                    auto check_op_structinsert(Register_usage_profile& register_usage_profile,
                                               Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *target);
                        assert_type_of_register<viua::internals::ValueTypes::STRUCT>(register_usage_profile,
                                                                                     *target);

                        auto key = get_operand<RegisterIndex>(instruction, 1);
                        if (not key) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *key);
                        assert_type_of_register<viua::internals::ValueTypes::ATOM>(register_usage_profile,
                                                                                   *key);

                        auto source = get_operand<RegisterIndex>(instruction, 2);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(2)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source);
                        erase_if_direct_access(register_usage_profile, source, instruction);
                    }
                    auto check_op_structremove(Register_usage_profile& register_usage_profile,
                                               Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            if (not get_operand<VoidLiteral>(instruction, 0)) {
                                throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                    .note("expected register index or void literal");
                            }
                        }

                        if (target) {
                            check_use_of_register(register_usage_profile, *target);
                        }

                        auto source = get_operand<RegisterIndex>(instruction, 1);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source);
                        assert_type_of_register<viua::internals::ValueTypes::STRUCT>(register_usage_profile,
                                                                                     *source);

                        auto key = get_operand<RegisterIndex>(instruction, 2);
                        if (not key) {
                            throw invalid_syntax(instruction.operands.at(2)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *key);
                        assert_type_of_register<viua::internals::ValueTypes::ATOM>(register_usage_profile,
                                                                                   *key);

                        if (target) {
                            register_usage_profile.define(Register{*target}, target->tokens.at(0));
                        }
                    }
                    auto check_op_structkeys(Register_usage_profile& register_usage_profile,
                                             Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_if_name_resolved(register_usage_profile, *target);

                        auto source = get_operand<RegisterIndex>(instruction, 1);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source);
                        assert_type_of_register<viua::internals::ValueTypes::STRUCT>(register_usage_profile,
                                                                                     *source);

                        auto val = Register{*target};
                        val.value_type = ValueTypes::VECTOR;
                        register_usage_profile.define(val, target->tokens.at(0));
                    }
                    auto check_op_msg(Register_usage_profile& register_usage_profile,
                                      Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            if (not get_operand<VoidLiteral>(instruction, 0)) {
                                throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                    .note("expected register index or void");
                            }
                        }

                        if (target) {
                            check_if_name_resolved(register_usage_profile, *target);
                        }

                        auto fn = instruction.operands.at(1).get();
                        if ((not dynamic_cast<AtomLiteral*>(fn)) and
                            (not dynamic_cast<FunctionNameLiteral*>(fn)) and
                            (not dynamic_cast<RegisterIndex*>(fn))) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected function name, atom literal, or register index");
                        }
                        if (auto r = dynamic_cast<RegisterIndex*>(fn); r) {
                            check_use_of_register(register_usage_profile, *r, "call from");
                            assert_type_of_register<viua::internals::ValueTypes::INVOCABLE>(
                                register_usage_profile, *r);
                        }

                        if (target) {
                            register_usage_profile.define(Register{*target}, target->tokens.at(0));
                        }
                    }
                    auto check_op_insert(Register_usage_profile& register_usage_profile,
                                         Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *target);
                        assert_type_of_register<viua::internals::ValueTypes::OBJECT>(register_usage_profile,
                                                                                     *target);

                        auto key = get_operand<RegisterIndex>(instruction, 1);
                        if (not key) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *key);
                        assert_type_of_register<viua::internals::ValueTypes::STRING>(register_usage_profile,
                                                                                     *key);

                        auto source = get_operand<RegisterIndex>(instruction, 2);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(2)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source);
                        erase_if_direct_access(register_usage_profile, source, instruction);
                    }
                    auto check_op_remove(Register_usage_profile& register_usage_profile,
                                         Instruction const& instruction) -> void {
                        auto target = get_operand<RegisterIndex>(instruction, 0);
                        if (not target) {
                            if (not get_operand<VoidLiteral>(instruction, 0)) {
                                throw invalid_syntax(instruction.operands.at(0)->tokens, "invalid operand")
                                    .note("expected register index or void literal");
                            }
                        }

                        auto source = get_operand<RegisterIndex>(instruction, 1);
                        if (not source) {
                            throw invalid_syntax(instruction.operands.at(1)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *source);
                        assert_type_of_register<viua::internals::ValueTypes::OBJECT>(register_usage_profile,
                                                                                     *source);

                        auto key = get_operand<RegisterIndex>(instruction, 2);
                        if (not key) {
                            throw invalid_syntax(instruction.operands.at(2)->tokens, "invalid operand")
                                .note("expected register index");
                        }

                        check_use_of_register(register_usage_profile, *key);
                        assert_type_of_register<viua::internals::ValueTypes::STRING>(register_usage_profile,
                                                                                     *key);

                        if (target) {
                            register_usage_profile.define(Register{*target}, target->tokens.at(0));
                        }
                    }
                }  // namespace checkers
            }      // namespace static_analyser
        }          // namespace frontend
    }              // namespace assembler
}  // namespace viua
