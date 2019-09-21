/*
 *  Copyright (C) 2017-2019 Marek Marecki
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

#ifndef VIUA_ASSEMBLER_FRONTEND_STATIC_ANALYSER_H
#define VIUA_ASSEMBLER_FRONTEND_STATIC_ANALYSER_H


#include <optional>
#include <set>
#include <string>
#include <viua/assembler/frontend/parser.h>


namespace viua { namespace assembler { namespace frontend {
namespace static_analyser {
auto verify_block_tries(parser::Parsed_source const&) -> void;
auto verify_block_catches(parser::Parsed_source const&) -> void;
auto verify_block_endings(parser::Parsed_source const& src) -> void;
auto verify_frame_balance(parser::Parsed_source const& src) -> void;
auto verify_function_call_arities(parser::Parsed_source const& src) -> void;
auto verify_frames_have_no_gaps(parser::Parsed_source const& src) -> void;
auto verify_jumps_are_in_range(parser::Parsed_source const&) -> void;

auto verify(parser::Parsed_source const&) -> void;

struct Register {
    viua::internals::types::register_index index{0};
    viua::internals::Register_sets register_set =
        viua::internals::Register_sets::LOCAL;
    viua::internals::Value_types value_type =
        viua::internals::Value_types::UNDEFINED;
    std::pair<bool, viua::cg::lex::Token> inferred = {false, {}};

    auto operator<(Register const& that) const -> bool;
    auto operator==(Register const& that) const -> bool;

    Register() = default;
    Register(viua::assembler::frontend::parser::Register_index const&);
};
auto to_string(Register const&) -> std::string;

struct Closure {
    std::string name;
    std::map<Register, std::pair<viua::cg::lex::Token, Register>>
        defined_registers;

    auto define(Register const, viua::cg::lex::Token const) -> void;

    Closure();
    Closure(std::string);
};

class Register_usage_profile {
    /*
     * Maps a register to the token that "defined" the register.
     * Consider:
     *
     *      text %1 local "Hello World!"    ; register 1 is defined, and
     *                                      ; the defining token is '%1'
     */
    std::map<Register, std::pair<viua::cg::lex::Token, Register>>
        defined_registers;

    /*
     * Registers are "fresh" until they either 1/ cross the boundary of an "if"
     * instruction, or 2/ are used. After that they are no longer fresh.
     * Registers in `arguments` register set are fresh until a calling
     * instruction is executed (e.g. `call`, or `process`). Overwriting a fresh
     * register is an error because it means that the previously defined value
     * is never used.
     */
    std::set<Register> fresh_registers;

    /*
     * Maps a register to the token marking the last place a register
     * has been used.
     * Consider:
     *
     *      text %1 local "Hello World!"    ; register 1 is defined, but unused
     *      print %1 local                  ; register 1 becomes used
     */
    std::map<Register, viua::cg::lex::Token> used_registers;

    /*
     * Maps a register to the location where it was erased.
     * Note that not every register must be erased, and
     * when a register becomes defined it is no longer erased.
     * Consider:
     *
     *      text %1 local "Hello World!"    ; register 1 is defined
     *      delete %1 local                 ; register 1 is erased
     *      integer %1 local 42             ; register 1 is defined again
     */
    std::map<Register, viua::cg::lex::Token> erased_registers;

    /*
     * The set of registers marked as "maybe unused".
     * If a register is unused at the end of a block, but has an entry in
     * this set an error should not be thrown for this register (but a warning,
     * or a different type of diagnostic message can be emitted).
     */
    std::set<Register> maybe_unused_registers;

    std::optional<viua::internals::types::register_index>
        no_of_allocated_registers;
    std::optional<viua::cg::lex::Token> where_registers_were_allocated;

    auto fresh(Register const) const -> bool;

  public:
    std::map<std::string, viua::internals::types::register_index> name_to_index;
    std::map<viua::internals::types::register_index, std::string> index_to_name;

    auto define(Register const r,
                viua::cg::lex::Token const t,
                bool const = false) -> void;
    auto define(Register const r,
                viua::cg::lex::Token const index,
                viua::cg::lex::Token const register_set,
                bool const = false) -> void;
    auto defined(Register const r) const -> bool;
    auto defined_where(Register const r) const -> viua::cg::lex::Token;

    auto infer(Register const r,
               viua::internals::Value_types const value_type_id,
               viua::cg::lex::Token const& t) -> void;

    auto at(Register const r) const -> const
        decltype(defined_registers)::mapped_type;

    auto used(Register const r) const -> bool;
    auto use(Register const r, viua::cg::lex::Token const t) -> void;

    auto defresh() -> void;
    auto erase_arguments(viua::cg::lex::Token const) -> void;

    auto erase(Register const r, viua::cg::lex::Token const& token) -> void;
    auto erased(Register const r) const -> bool;
    auto erased_where(Register const r) const -> viua::cg::lex::Token;

    auto allocated_registers(viua::internals::types::register_index const)
        -> void;
    auto allocated_registers() const
        -> std::optional<viua::internals::types::register_index>;
    auto allocated_where(viua::cg::lex::Token const&) -> void;
    auto allocated_where() const -> std::optional<viua::cg::lex::Token>;

    auto in_bounds(Register const) const -> bool;

    auto begin() const -> decltype(defined_registers.begin());
    auto end() const -> decltype(defined_registers.end());
};

enum class Reportable_error {
    Unused_register,
    Unused_value,
};
auto to_reportable_error(std::string_view const)
    -> std::optional<Reportable_error>;
auto allowed_error(Reportable_error const) -> bool;
auto allow_error(Reportable_error const, bool const = true) -> void;

namespace checkers {
using viua::assembler::frontend::parser::Instruction;
using viua::assembler::frontend::parser::Instructions_block;
using viua::assembler::frontend::parser::Parsed_source;
using InstructionIndex = Instructions_block::size_type;

auto to_string(viua::internals::Register_sets const) -> std::string;
auto to_string(viua::internals::Value_types const) -> std::string;

template<typename T>
auto get_operand(
    viua::assembler::frontend::parser::Instruction const& instruction,
    size_t operand_index) -> T* {
    if (operand_index >= instruction.operands.size()) {
        using viua::cg::lex::Invalid_syntax;
        throw Invalid_syntax{instruction.tokens.at(0), "not enough operands"}
            .note("when extracting operand " + std::to_string(operand_index));
    }
    T* operand = dynamic_cast<T*>(instruction.operands.at(operand_index).get());
    return operand;
}

auto invalid_syntax(std::vector<viua::cg::lex::Token> const&, std::string const)
    -> viua::cg::lex::Invalid_syntax;
using viua::assembler::frontend::parser::Register_index;
auto check_if_name_resolved(Register_usage_profile const& rup,
                            Register_index const r) -> void;
auto check_use_of_register(Register_usage_profile&,
                           viua::assembler::frontend::parser::Register_index,
                           std::string const           = "use of",
                           bool const allow_arguments  = false,
                           bool const allow_parameters = false) -> void;


using viua::internals::Value_types;
auto operator|(Value_types const, Value_types const) -> Value_types;
auto operator&(Value_types const, Value_types const) -> Value_types;
auto operator^(Value_types const, Value_types const) -> Value_types;
auto operator!(Value_types const) -> bool;

auto depointerise_type_if_needed(viua::internals::Value_types const, bool const)
    -> viua::internals::Value_types;

template<viua::internals::Value_types expected_type>
auto assert_type_of_register(Register_usage_profile& register_usage_profile,
                             Register_index const& register_index)
    -> viua::internals::Value_types {
    using viua::cg::lex::Invalid_syntax;
    using viua::cg::lex::Traced_syntax_error;
    using viua::internals::Value_types;

    if (register_index.rss == viua::internals::Register_sets::GLOBAL
        or register_index.rss == viua::internals::Register_sets::STATIC) {
        return expected_type;
    }

    auto actual_type =
        register_usage_profile.at(Register(register_index)).second.value_type;

    auto access_via_pointer_dereference =
        (register_index.as
         == viua::internals::Access_specifier::POINTER_DEREFERENCE);
    if (access_via_pointer_dereference) {
        /*
         * Throw only if the type is not UNDEFINED.
         * If the type is UNDEFINED let the inferencer do its job.
         */
        if ((actual_type != Value_types::UNDEFINED)
            and not(actual_type & Value_types::POINTER)) {
            auto error =
                viua::cg::lex::Traced_syntax_error{}
                    .append(Invalid_syntax(
                                register_index.tokens.at(0),
                                "invalid type of value contained in register "
                                "for this access type")
                                .note("need pointer to "
                                      + to_string(expected_type) + ", got "
                                      + to_string(actual_type)))
                    .append(Invalid_syntax(register_usage_profile.defined_where(
                                               Register(register_index)),
                                           "")
                                .note("register defined here"));
            throw error;
        }

        /*
         * Modify types only if they are defined.
         * Tinkering with UNDEFINEDs will prevent inferencer from kicking in.
         */
        if (actual_type != Value_types::UNDEFINED) {
            actual_type = (actual_type ^ Value_types::POINTER);
        }
    }

    if (actual_type == Value_types::UNDEFINED) {
        auto inferred_type =
            (expected_type
             | (access_via_pointer_dereference ? Value_types::POINTER
                                               : Value_types::UNDEFINED));
        register_usage_profile.infer(Register(register_index),
                                     inferred_type,
                                     register_index.tokens.at(0));
        return depointerise_type_if_needed(inferred_type,
                                           access_via_pointer_dereference);
    }

    if (expected_type == Value_types::UNDEFINED) {
        return depointerise_type_if_needed(actual_type,
                                           access_via_pointer_dereference);
    }
    if (not(actual_type & expected_type)) {
        auto error =
            Traced_syntax_error{}
                .append(Invalid_syntax(
                            register_index.tokens.at(0),
                            "invalid type of value contained in register")
                            .note("expected " + to_string(expected_type)
                                  + ", got " + to_string(actual_type)))
                .append(Invalid_syntax(register_usage_profile.defined_where(
                                           Register(register_index)),
                                       "")
                            .note("register defined here"));
        if (auto r = register_usage_profile.at(Register(register_index)).second;
            r.inferred.first) {
            error.append(Invalid_syntax(r.inferred.second, "")
                             .note("type inferred here")
                             .aside(r.inferred.second,
                                    "deduced type is '"
                                        + to_string(r.value_type) + "'"));
        }
        throw error;
    }

    return depointerise_type_if_needed(actual_type,
                                       access_via_pointer_dereference);
}

template<typename T>
auto get_input_operand(
    viua::assembler::frontend::parser::Instruction const& instruction,
    size_t operand_index) -> T* {
    auto operand = get_operand<T>(instruction, operand_index);
    if ((not operand)
        and dynamic_cast<viua::assembler::frontend::parser::Void_literal*>(
                instruction.operands.at(operand_index).get())) {
        throw viua::cg::lex::Invalid_syntax{
            instruction.operands.at(operand_index)->tokens.at(0),
            "use of void as input register:"};
    }
    return operand;
}

auto check_op_izero(Register_usage_profile& register_usage_profile,
                    Instruction const& instruction) -> void;
auto check_op_integer(Register_usage_profile& register_usage_profile,
                      Instruction const& instruction) -> void;
auto check_op_iinc(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_bit_increment(Register_usage_profile& register_usage_profile,
                            Instruction const& instruction) -> void;
auto check_op_bit_arithmetic(Register_usage_profile& register_usage_profile,
                             Instruction const& instruction) -> void;
auto check_op_float(Register_usage_profile& register_usage_profile,
                    Instruction const& instruction) -> void;
auto check_op_itof(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_ftoi(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_stoi(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_stof(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_arithmetic(Register_usage_profile& register_usage_profile,
                         Instruction const& instruction) -> void;
auto check_op_compare(Register_usage_profile& register_usage_profile,
                      Instruction const& instruction) -> void;
auto check_op_string(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void;
auto check_op_streq(Register_usage_profile& register_usage_profile,
                    Instruction const& instruction) -> void;
auto check_op_text(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_texteq(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void;
auto check_op_textat(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void;
auto check_op_textsub(Register_usage_profile& register_usage_profile,
                      Instruction const& instruction) -> void;
auto check_op_textlength(Register_usage_profile& register_usage_profile,
                         Instruction const& instruction) -> void;
auto check_op_textcommonprefix(Register_usage_profile& register_usage_profile,
                               Instruction const& instruction) -> void;
auto check_op_textcommonsuffix(Register_usage_profile& register_usage_profile,
                               Instruction const& instruction) -> void;
auto check_op_textconcat(Register_usage_profile& register_usage_profile,
                         Instruction const& instruction) -> void;
auto check_op_vector(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void;
auto check_op_vinsert(Register_usage_profile& register_usage_profile,
                      Instruction const& instruction) -> void;
auto check_op_vpush(Register_usage_profile& register_usage_profile,
                    Instruction const& instruction) -> void;
auto check_op_vpop(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_vat(Register_usage_profile& register_usage_profile,
                  Instruction const& instruction) -> void;
auto check_op_vlen(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_not(Register_usage_profile& register_usage_profile,
                  Instruction const& instruction) -> void;
auto check_op_boolean_and_or(Register_usage_profile& register_usage_profile,
                             Instruction const& instruction) -> void;
auto check_op_bits_of_integer(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_integer_of_bits(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_bits(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_binary_logic(Register_usage_profile& register_usage_profile,
                           Instruction const& instruction) -> void;
auto check_op_bitnot(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void;
auto check_op_bitat(Register_usage_profile& register_usage_profile,
                    Instruction const& instruction) -> void;
auto check_op_bitset(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void;
auto check_op_bit_shifts(Register_usage_profile& register_usage_profile,
                         Instruction const& instruction) -> void;
auto check_op_bit_rotates(Register_usage_profile& register_usage_profile,
                          Instruction const& instruction) -> void;
auto check_op_copy(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_move(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_ptr(Register_usage_profile& register_usage_profile,
                  Instruction const& instruction) -> void;
auto check_op_ptrlive(Register_usage_profile& register_usage_profile,
                      Instruction const& instruction) -> void;
auto check_op_swap(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_delete(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void;
auto check_op_isnull(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void;
auto check_op_print(Register_usage_profile& register_usage_profile,
                    Instruction const& instruction) -> void;
auto check_op_capture(Register_usage_profile& register_usage_profile,
                      Instruction const& instruction,
                      std::map<Register, Closure>& created_closures) -> void;
auto check_op_capturecopy(Register_usage_profile& register_usage_profile,
                          Instruction const& instruction,
                          std::map<Register, Closure>& created_closures)
    -> void;
auto check_op_capturemove(Register_usage_profile& register_usage_profile,
                          Instruction const& instruction,
                          std::map<Register, Closure>& created_closures)
    -> void;
auto check_op_closure(Register_usage_profile& register_usage_profile,
                      Instruction const& instruction,
                      std::map<Register, Closure>& created_closures) -> void;
auto check_op_function(Register_usage_profile& register_usage_profile,
                       Instruction const& instruction) -> void;
auto check_op_frame(Register_usage_profile&, Instruction const&) -> void;
auto check_op_param(Register_usage_profile& register_usage_profile,
                    Instruction const& instruction) -> void;
auto check_op_call(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_tailcall(Register_usage_profile& register_usage_profile,
                       Instruction const& instruction) -> void;
auto check_op_defer(Register_usage_profile& register_usage_profile,
                    Instruction const& instruction) -> void;
auto check_op_allocate_registers(Register_usage_profile& register_usage_profile,
                                 Instruction const& instruction) -> void;
auto check_op_process(Register_usage_profile& register_usage_profile,
                      Instruction const& instruction) -> void;
auto check_op_self(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_pideq(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void;
auto check_op_join(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_send(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_receive(Register_usage_profile& register_usage_profile,
                      Instruction const& instruction) -> void;
auto check_op_watchdog(Register_usage_profile&, Instruction const& instruction)
    -> void;
auto check_op_throw(Register_usage_profile& register_usage_profile,
                    Parsed_source const& ps,
                    std::map<Register, Closure>&,
                    Instruction const&) -> void;
auto check_op_draw(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_enter(Register_usage_profile& register_usage_profile,
                    Parsed_source const& ps,
                    Instruction const& instruction) -> void;
auto check_op_jump(Register_usage_profile& register_usage_profile,
                   Parsed_source const& ps,
                   Instruction const& instruction,
                   Instructions_block const& ib,
                   InstructionIndex i,
                   InstructionIndex const mnemonic_counter) -> void;
auto check_op_if(Register_usage_profile& register_usage_profile,
                 Parsed_source const& ps,
                 Instruction const& instruction,
                 Instructions_block const& ib,
                 InstructionIndex i,
                 InstructionIndex const mnemonic_counter) -> void;
auto check_op_atom(Register_usage_profile& register_usage_profile,
                   Instruction const& instruction) -> void;
auto check_op_atomeq(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void;
auto check_op_struct(Register_usage_profile& register_usage_profile,
                     Instruction const& instruction) -> void;
auto check_op_structinsert(Register_usage_profile& register_usage_profile,
                           Instruction const& instruction) -> void;
auto check_op_structremove(Register_usage_profile& register_usage_profile,
                           Instruction const& instruction) -> void;
auto check_op_structat(Register_usage_profile& register_usage_profile,
                       Instruction const& instruction) -> void;
auto check_op_structkeys(Register_usage_profile& register_usage_profile,
                         Instruction const& instruction) -> void;
auto check_op_io_read(Register_usage_profile& register_usage_profile,
                         Instruction const& instruction) -> void;
auto check_op_io_write(Register_usage_profile& register_usage_profile,
                         Instruction const& instruction) -> void;
auto check_op_io_wait(Register_usage_profile& register_usage_profile,
                         Instruction const& instruction) -> void;
auto check_op_io_cancel(Register_usage_profile& register_usage_profile,
                         Instruction const& instruction) -> void;
auto check_op_io_close(Register_usage_profile& register_usage_profile,
                         Instruction const& instruction) -> void;

auto check_for_unused_registers(
    Register_usage_profile const& register_usage_profile) -> void;
auto check_for_unused_values(
    Register_usage_profile const& register_usage_profile) -> void;
auto check_closure_instantiations(
    Register_usage_profile const& register_usage_profile,
    Parsed_source const& ps,
    std::map<Register, Closure> const& created_closures) -> void;

auto check_register_usage_for_instruction_block_impl(Register_usage_profile&,
                                                     Parsed_source const&,
                                                     Instructions_block const&,
                                                     InstructionIndex,
                                                     InstructionIndex) -> void;

auto map_names_to_register_indexes(Register_usage_profile&,
                                   Instructions_block const&) -> void;
auto erase_if_direct_access(
    Register_usage_profile&,
    Register_index* const,
    viua::assembler::frontend::parser::Instruction const&) -> void;
auto get_line_index_of_instruction(InstructionIndex const,
                                   Instructions_block const&)
    -> InstructionIndex;
}  // namespace checkers

auto check_register_usage(parser::Parsed_source const&) -> void;
}}}}  // namespace viua::assembler::frontend::static_analyser

#endif
