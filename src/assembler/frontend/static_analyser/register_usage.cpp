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

#include <iostream>
#include <map>
#include <set>
#include <viua/assembler/frontend/static_analyser.h>
#include <viua/bytecode/operand_types.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/support/string.h>
#include <viua/util/range.h>
using namespace std;
using namespace viua::assembler::frontend::parser;


using viua::cg::lex::Invalid_syntax;
using viua::cg::lex::Token;
using viua::cg::lex::Traced_syntax_error;

using viua::assembler::frontend::static_analyser::Closure;
using viua::assembler::frontend::static_analyser::Register;
using viua::assembler::frontend::static_analyser::Register_usage_profile;


using Verifier = auto (*)(Parsed_source const&, Instructions_block const&)
                     -> void;
static auto verify_wrapper(Parsed_source const& source, Verifier verifier)
    -> void {
    for (auto const& fn : source.functions) {
        if (fn.attributes.count("no_sa")) {
            continue;
        }
        try {
            verifier(source, fn);
        } catch (Invalid_syntax& e) {
            throw viua::cg::lex::Traced_syntax_error{}.append(e).append(
                Invalid_syntax{fn.name, ("in function " + fn.name.str())});
        } catch (Traced_syntax_error& e) {
            throw e.append(
                Invalid_syntax{fn.name, ("in function " + fn.name.str())});
        }
    }
}

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
auto map_names_to_register_indexes(
    Register_usage_profile& register_usage_profile,
    Instructions_block const& ib) -> void {
    for (auto const& line : ib.body) {
        auto directive =
            dynamic_cast<viua::assembler::frontend::parser::Directive*>(
                line.get());
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
        auto index =
            static_cast<viua::internals::types::register_index>(stoul(idx));
        auto name = directive->operands.at(1);

        if (str::is_register_set_name(name)) {
            throw Invalid_syntax{directive->tokens.at(0),
                                 "invalid register name: " + name}
                .add(directive->tokens.at(2))
                .note("'" + name + "' is a register set name");
        }

        if (register_usage_profile.name_to_index.count(name)) {
            throw Invalid_syntax{directive->tokens.at(2),
                                 "register name already taken: " + name}
                .add(directive->tokens.at(0));
        }

        register_usage_profile.name_to_index[name]  = index;
        register_usage_profile.index_to_name[index] = name;
    }
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers

using InstructionIndex = Instructions_block::size_type;

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
auto check_register_usage_for_instruction_block_impl(
    Register_usage_profile& register_usage_profile,
    Parsed_source const& ps,
    Instructions_block const& ib,
    InstructionIndex i,
    InstructionIndex mnemonic_counter) -> void {
    using namespace viua::assembler::frontend::static_analyser::checkers;

    std::map<Register, Closure> created_closures;

    for (auto stop = false; (not stop) and (i < ib.body.size()); ++i) {
        auto const& line = ib.body.at(i);
        auto const instruction =
            dynamic_cast<viua::assembler::frontend::parser::Instruction*>(
                line.get());
        if (not instruction) {
            continue;
        }
        ++mnemonic_counter;

        using viua::assembler::frontend::parser::Register_index;
        auto const opcode = instruction->opcode;

        try {
            switch (opcode) {
            case IZERO:
                check_op_izero(register_usage_profile, *instruction);
                break;
            case INTEGER:
                check_op_integer(register_usage_profile, *instruction);
                break;
            case IINC:
            case IDEC:
                check_op_iinc(register_usage_profile, *instruction);
                break;
            case WRAPINCREMENT:
            case WRAPDECREMENT:
            case CHECKEDSINCREMENT:
            case CHECKEDSDECREMENT:
            case SATURATINGSINCREMENT:
            case SATURATINGSDECREMENT:
                check_op_bit_increment(register_usage_profile, *instruction);
                break;
            case FLOAT:
                check_op_float(register_usage_profile, *instruction);
                break;
            case ITOF:
                check_op_itof(register_usage_profile, *instruction);
                break;
            case FTOI:
                check_op_ftoi(register_usage_profile, *instruction);
                break;
            case STOI:
                check_op_stoi(register_usage_profile, *instruction);
                break;
            case STOF:
                check_op_stof(register_usage_profile, *instruction);
                break;
            case ADD:
            case SUB:
            case MUL:
            case DIV:
                check_op_arithmetic(register_usage_profile, *instruction);
                break;
            case WRAPADD:
            case WRAPSUB:
            case WRAPMUL:
            case WRAPDIV:
            case CHECKEDSADD:
            case CHECKEDSSUB:
            case CHECKEDSMUL:
            case CHECKEDSDIV:
            case SATURATINGSADD:
            case SATURATINGSSUB:
            case SATURATINGSMUL:
            case SATURATINGSDIV:
                check_op_bit_arithmetic(register_usage_profile, *instruction);
                break;
            case CHECKEDUINCREMENT:
            case CHECKEDUDECREMENT:
            case CHECKEDUADD:
            case CHECKEDUSUB:
            case CHECKEDUMUL:
            case CHECKEDUDIV:
            case SATURATINGUINCREMENT:
            case SATURATINGUDECREMENT:
            case SATURATINGUADD:
            case SATURATINGUSUB:
            case SATURATINGUMUL:
            case SATURATINGUDIV:
                // FIXME implement
                break;
            case LT:
            case LTE:
            case GT:
            case GTE:
            case EQ:
                check_op_compare(register_usage_profile, *instruction);
                break;
            case STRING:
                check_op_string(register_usage_profile, *instruction);
                break;
            case STREQ:
                check_op_streq(register_usage_profile, *instruction);
                break;
            case TEXT:
                check_op_text(register_usage_profile, *instruction);
                break;
            case TEXTEQ:
                check_op_texteq(register_usage_profile, *instruction);
                break;
            case TEXTAT:
                check_op_textat(register_usage_profile, *instruction);
                break;
            case TEXTSUB:
                check_op_textsub(register_usage_profile, *instruction);
                break;
            case TEXTLENGTH:
                check_op_textlength(register_usage_profile, *instruction);
                break;
            case TEXTCOMMONPREFIX:
                check_op_textcommonprefix(register_usage_profile, *instruction);
                break;
            case TEXTCOMMONSUFFIX:
                check_op_textcommonsuffix(register_usage_profile, *instruction);
                break;
            case TEXTCONCAT:
                check_op_textconcat(register_usage_profile, *instruction);
                break;
            case VECTOR:
                check_op_vector(register_usage_profile, *instruction);
                break;
            case VINSERT:
                check_op_vinsert(register_usage_profile, *instruction);
                break;
            case VPUSH:
                check_op_vpush(register_usage_profile, *instruction);
                break;
            case VPOP:
                check_op_vpop(register_usage_profile, *instruction);
                break;
            case VAT:
                check_op_vat(register_usage_profile, *instruction);
                break;
            case VLEN:
                check_op_vlen(register_usage_profile, *instruction);
                break;
            case NOT:
                check_op_not(register_usage_profile, *instruction);
                break;
            case AND:
            case OR:
                check_op_boolean_and_or(register_usage_profile, *instruction);
                break;
            case BITS:
                check_op_bits(register_usage_profile, *instruction);
                break;
            case BITAND:
            case BITOR:
            case BITXOR:
                check_op_binary_logic(register_usage_profile, *instruction);
                break;
            case BITNOT:
                check_op_bitnot(register_usage_profile, *instruction);
                break;
            case BITAT:
                check_op_bitat(register_usage_profile, *instruction);
                break;
            case BITSET:
                check_op_bitset(register_usage_profile, *instruction);
                break;
            case BITSWIDTH:
            case BITSEQ:
            case BITSLT:
            case BITSLTE:
            case BITSGT:
            case BITSGTE:
            case BITAEQ:
            case BITALT:
            case BITALTE:
            case BITAGT:
            case BITAGTE:
                // do nothing
                break;
            case SHL:
            case SHR:
            case ASHL:
            case ASHR:
                check_op_bit_shifts(register_usage_profile, *instruction);
                break;
            case ROL:
            case ROR:
                check_op_bit_rotates(register_usage_profile, *instruction);
                break;
            case COPY:
                check_op_copy(register_usage_profile, *instruction);
                break;
            case MOVE:
                check_op_move(register_usage_profile, *instruction);
                break;
            case PTR:
                check_op_ptr(register_usage_profile, *instruction);
                break;
            case PTRLIVE:
                check_op_ptrlive(register_usage_profile, *instruction);
                break;
            case SWAP:
                check_op_swap(register_usage_profile, *instruction);
                break;
            case DELETE:
                check_op_delete(register_usage_profile, *instruction);
                break;
            case ISNULL:
                check_op_isnull(register_usage_profile, *instruction);
                break;
            case PRINT:
            case ECHO:
                check_op_print(register_usage_profile, *instruction);
                break;
            case CAPTURE:
                check_op_capture(
                    register_usage_profile, *instruction, created_closures);
                break;
            case CAPTURECOPY:
                check_op_capturecopy(
                    register_usage_profile, *instruction, created_closures);
                break;
            case CAPTUREMOVE:
                check_op_capturemove(
                    register_usage_profile, *instruction, created_closures);
                break;
            case CLOSURE:
                check_op_closure(
                    register_usage_profile, *instruction, created_closures);
                break;
            case FUNCTION:
                check_op_function(register_usage_profile, *instruction);
                break;
            case FRAME:
                check_op_frame(register_usage_profile, *instruction);
                break;
            case CALL:
                check_op_call(register_usage_profile, *instruction);
                break;
            case TAILCALL:
                check_op_tailcall(register_usage_profile, *instruction);

                /*
                 * Stop analysis after a tailcall because at this point the
                 * frame does no longer belong to the currently analysed
                 * function but to some other one. This sudden control transfer
                 * makes it pointless to continue analysing the current control
                 * flow - it is similar to a return of throw instruction in this
                 * regard.
                 */
                stop = true;
                break;
            case DEFER:
                check_op_defer(register_usage_profile, *instruction);
                break;
            case ALLOCATE_REGISTERS:
                check_op_allocate_registers(register_usage_profile,
                                            *instruction);
                break;
            case PROCESS:
                check_op_process(register_usage_profile, *instruction);
                break;
            case SELF:
                check_op_self(register_usage_profile, *instruction);
                break;
            case JOIN:
                check_op_join(register_usage_profile, *instruction);
                break;
            case SEND:
                check_op_send(register_usage_profile, *instruction);
                break;
            case RECEIVE:
                check_op_receive(register_usage_profile, *instruction);
                break;
            case WATCHDOG:
                check_op_watchdog(register_usage_profile, *instruction);
                break;
            case JUMP:
                check_op_jump(register_usage_profile,
                              ps,
                              *instruction,
                              ib,
                              i,
                              mnemonic_counter);
                /*
                 * Do not perform further checks after a "jump" instruction.
                 * All state has been "moved" into the nested checking call
                 * above, and it would be useless to perform the checks twice
                 * here.
                 */
                return;
            case IF:
                check_op_if(register_usage_profile,
                            ps,
                            *instruction,
                            ib,
                            i,
                            mnemonic_counter);
                /*
                 * Do not perform further checks after an "if" instruction.
                 * All state has been "moved" into the nested checking call
                 * above, and it would be useless to perform the checks twice
                 * here.
                 */
                return;
            case THROW:
                check_op_throw(
                    register_usage_profile, ps, created_closures, *instruction);
                break;
            case CATCH:
                // FIXME TODO SA for entered blocks
                break;
            case DRAW:
                check_op_draw(register_usage_profile, *instruction);
                break;
            case TRY:
                // do nothing
                break;
            case ENTER:
                check_op_enter(register_usage_profile, ps, *instruction);
                break;
            case LEAVE:
                /*
                 * Just return.
                 * Since blocks are never entered independently (and only in
                 * context of some other frame, we will use that "outer" frame
                 * to check for unused values, etc.
                 *
                 * This may lead to less-than-stellar readability for some
                 * errors; e.g. when a value is defined inside a block, but is
                 * used neither inside it nor in the surrounding frame.
                 */
                return;
            case IMPORT:
                // do nothing
                break;
            case ATOM:
                check_op_atom(register_usage_profile, *instruction);
                break;
            case ATOMEQ:
                check_op_atomeq(register_usage_profile, *instruction);
                break;
            case STRUCT:
                check_op_struct(register_usage_profile, *instruction);
                break;
            case STRUCTINSERT:
                check_op_structinsert(register_usage_profile, *instruction);
                break;
            case STRUCTREMOVE:
                check_op_structremove(register_usage_profile, *instruction);
                break;
            case STRUCTAT:
                check_op_structat(register_usage_profile, *instruction);
                break;
            case STRUCTKEYS:
                check_op_structkeys(register_usage_profile, *instruction);
                break;
            case RETURN:
                // do nothing
                break;
            case HALT:
                // do nothing
                break;
            case NOP:
                // do nothing
                break;
            case BOOL:
                // FIXME "bool" opcode is not even implemented
                break;
            case ARG:
            case PARAM:
            case PAMV:
                throw invalid_syntax(
                    instruction->tokens,
                    "instruction internal to the VM found in user code");
            default:
                throw invalid_syntax(
                    instruction->tokens,
                    "instruction does not have static analysis implemented");
            }
        } catch (Invalid_syntax& e) {
            throw e.add(instruction->tokens.at(0));
        } catch (Traced_syntax_error& e) {
            e.errors.at(0).add(instruction->tokens.at(0));
            throw e;
        }
    }

    /*
     * These checks should only be run if the SA actually reached the end of the
     * instructions block.
     */
    check_for_unused_registers(register_usage_profile);
    check_for_unused_values(register_usage_profile);
    check_closure_instantiations(register_usage_profile, ps, created_closures);
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers

static auto check_register_usage_for_instruction_block(
    Parsed_source const& ps,
    Instructions_block const& ib) -> void {
    if (ib.closure) {
        return;
    }

    auto const& first_line = ib.body.at(0);
    auto const instruction =
        dynamic_cast<viua::assembler::frontend::parser::Instruction*>(
            first_line.get());
    if (not instruction) {
        throw viua::cg::lex::Invalid_syntax{
            first_line->tokens.at(0),
            "first instruction must be 'allocate_registers' for local "
            "registers"};
    }
    if (instruction->opcode != ALLOCATE_REGISTERS) {
        throw viua::cg::lex::Invalid_syntax{
            first_line->tokens.at(0),
            "first instruction must be 'allocate_registers' for local "
            "registers"};
    }

    Register_usage_profile register_usage_profile;
    viua::assembler::frontend::static_analyser::checkers::
        map_names_to_register_indexes(register_usage_profile, ib);

    // FIXME: This is ad-hoc code - move it to a utility function.
    auto const function_arity =
        std::stoul(ib.name.str().substr(ib.name.str().rfind('/') + 1));
    for (auto const each :
         viua::util::Range(static_cast<viua::internals::types::register_index>(
             function_arity))) {
        auto val         = Register{};
        val.index        = each;
        val.register_set = viua::internals::Register_sets::PARAMETERS;
        register_usage_profile.define(val, ib.name);
    }

    viua::assembler::frontend::static_analyser::checkers::
        check_register_usage_for_instruction_block_impl(
            register_usage_profile,
            ps,
            ib,
            0,
            static_cast<InstructionIndex>(-1));
}
auto viua::assembler::frontend::static_analyser::check_register_usage(
    Parsed_source const& src) -> void {
    verify_wrapper(src, check_register_usage_for_instruction_block);
}
