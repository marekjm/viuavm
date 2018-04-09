/*
 *  Copyright (C) 2017, 2018 Marek Marecki
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

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <viua/assembler/util/pretty_printer.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/tokenizer.h>
#include <viua/cg/tools.h>
#include <viua/front/asm.h>
#include <viua/loader.h>
#include <viua/machine.h>
#include <viua/assembler/backend/op_assemblers.h>
#include <viua/program.h>
#include <viua/support/env.h>
#include <viua/support/string.h>
using namespace std;


extern bool DEBUG;
extern bool SCREAM;

using viua::assembler::util::pretty_printer::ATTR_RESET;
using viua::assembler::util::pretty_printer::COLOR_FG_LIGHT_GREEN;
using viua::assembler::util::pretty_printer::COLOR_FG_WHITE;
using viua::assembler::util::pretty_printer::send_control_seq;


using Token      = viua::cg::lex::Token;
using viua::assembler::backend::op_assemblers::Token_index;


auto ::assembler::operands::resolve_jump(
    Token token,
    std::map<std::string, std::vector<Token>::size_type> const& marks,
    viua::internals::types::bytecode_size instruction_index) -> std::tuple<viua::internals::types::bytecode_size, enum JUMPTYPE> {
    /*  This function is used to resolve jumps in `jump` and `branch`
     * instructions.
     */
    string jmp                                 = token.str();
    viua::internals::types::bytecode_size addr = 0;
    enum JUMPTYPE jump_type                    = JMP_RELATIVE;
    if (str::isnum(jmp, false)) {
        addr = stoul(jmp);
    } else if (jmp.substr(0, 2) == "0x") {
        stringstream ss;
        ss << hex << jmp;
        ss >> addr;
        jump_type = JMP_TO_BYTE;
    } else if (jmp[0] == '-') {
        int jump_value = stoi(jmp);
        if (instruction_index < static_cast<decltype(addr)>(-1 * jump_value)) {
            // FIXME: move jump verification to assembler::verify namespace
            // function
            ostringstream oss;
            oss << "use of relative jump results in a jump to negative index: ";
            oss << "jump_value = " << jump_value << ", ";
            oss << "instruction_index = " << instruction_index;
            throw viua::cg::lex::InvalidSyntax(token, oss.str());
        }
        addr = (instruction_index
                - static_cast<viua::internals::types::bytecode_size>(
                      -1 * jump_value));
    } else if (jmp[0] == '+') {
        addr = (instruction_index + stoul(jmp.substr(1)));
    } else {
        try {
            // FIXME: markers map should use
            // viua::internals::types::bytecode_size to avoid the need for
            // casting
            addr = static_cast<viua::internals::types::bytecode_size>(
                marks.at(jmp));
        } catch (const std::out_of_range& e) {
            throw viua::cg::lex::InvalidSyntax(
                token,
                ("cannot resolve jump to unrecognised marker: "
                 + str::enquote(str::strencode(jmp))));
        }
    }

    // FIXME: check if the jump is within the size of bytecode
    return tuple<viua::internals::types::bytecode_size, enum JUMPTYPE>(
        addr, jump_type);
}

auto ::assembler::operands::resolve_register(Token const token,
                              bool const allow_bare_integers) -> std::string {
    /*  This function is used to register numbers when a register is accessed,
     * e.g. in `integer` instruction or in `branch` in condition operand.
     *
     *  This function MUST return string as teh result is further passed to
     * assembler::operands::getint() function which *expects* string.
     */
    auto out = ostringstream{};
    auto const reg = token.str();
    if (reg[0] == '@' and str::isnum(str::sub(reg, 1))) {
        /*  Basic case - the register index is taken from another register,
         * everything is still nice and simple.
         */
        if (stoi(reg.substr(1)) < 0) {
            throw("register indexes cannot be negative: " + reg);
        }

        // FIXME: analyse source and detect if the referenced register really
        // holds an integer (the only value suitable to use as register
        // reference)
        out.str(reg);
    } else if (reg[0] == '*' and str::isnum(str::sub(reg, 1))) {
        /*  Basic case - the register index is taken from another register,
         * everything is still nice and simple.
         */
        if (stoi(reg.substr(1)) < 0) {
            throw("register indexes cannot be negative: " + reg);
        }

        out.str(reg);
    } else if (reg[0] == '%' and str::isnum(str::sub(reg, 1))) {
        /*  Basic case - the register index is taken from another register,
         * everything is still nice and simple.
         */
        if (stoi(reg.substr(1)) < 0) {
            throw("register indexes cannot be negative: " + reg);
        }

        out.str(reg);
    } else if (reg == "void") {
        out << reg;
    } else if (allow_bare_integers and str::isnum(reg)) {
        out << reg;
    } else {
        throw viua::cg::lex::InvalidSyntax(
            token,
            ("cannot resolve register operand: "
             + str::enquote(str::strencode(token.str()))));
    }
    return out.str();
}

auto ::assembler::operands::resolve_rs_type(Token const token) -> viua::internals::RegisterSets {
    if (token == "current") {
        return viua::internals::RegisterSets::CURRENT;
    } else if (token == "local") {
        return viua::internals::RegisterSets::LOCAL;
    } else if (token == "static") {
        return viua::internals::RegisterSets::STATIC;
    } else if (token == "global") {
        return viua::internals::RegisterSets::GLOBAL;
    } else {
        throw viua::cg::lex::InvalidSyntax(token,
                                           "invalid register set type name");
    }
}

static viua::internals::types::timeout timeout_to_int(const string& timeout) {
    const auto timeout_str_size = timeout.size();
    if (timeout[timeout_str_size - 2] == 'm') {
        return static_cast<viua::internals::types::timeout>(
            stoi(timeout.substr(0, timeout_str_size - 2)));
    } else {
        return static_cast<viua::internals::types::timeout>(
            (stoi(timeout.substr(0, timeout_str_size - 1)) * 1000));
    }
}

static auto convert_token_to_timeout_operand(viua::cg::lex::Token token)
    -> timeout_op {
    viua::internals::types::timeout timeout_milliseconds = 0;
    if (token != "infinity") {
        timeout_milliseconds = timeout_to_int(token);
        ++timeout_milliseconds;
    }
    return timeout_op{timeout_milliseconds};
}
static auto log_location_being_assembled(const Token& token) -> void {
    if (DEBUG and SCREAM) {
        cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << "debug"
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "<file>:" << token.line() << ':' << token.character();
        cout << ": assembling '";
        cout << send_control_seq(COLOR_FG_WHITE) << token.str()
             << send_control_seq(ATTR_RESET);
        cout << "' instruction\n";
    }
}

using viua::assembler::backend::op_assemblers::assemble_single_register_op;
using viua::assembler::backend::op_assemblers::assemble_double_register_op;
using viua::assembler::backend::op_assemblers::assemble_three_register_op;
using viua::assembler::backend::op_assemblers::assemble_four_register_op;
using viua::assembler::backend::op_assemblers::assemble_capture_op;
using viua::assembler::backend::op_assemblers::assemble_fn_ctor_op;
using viua::assembler::backend::op_assemblers::assemble_bit_shift_instruction;
using viua::assembler::backend::op_assemblers::assemble_increment_instruction;
using viua::assembler::backend::op_assemblers::assemble_arithmetic_instruction;

using viua::assembler::backend::op_assemblers::assemble_op_integer;
using viua::assembler::backend::op_assemblers::assemble_op_vinsert;
using viua::assembler::backend::op_assemblers::assemble_op_vpop;
using viua::assembler::backend::op_assemblers::assemble_op_bits;
using viua::assembler::backend::op_assemblers::assemble_op_bitset;
using viua::assembler::backend::op_assemblers::assemble_op_call;
using viua::assembler::backend::op_assemblers::assemble_op_if;
using viua::assembler::backend::op_assemblers::assemble_op_jump;
using viua::assembler::backend::op_assemblers::assemble_op_structremove;
using viua::assembler::backend::op_assemblers::assemble_op_msg;
using viua::assembler::backend::op_assemblers::assemble_op_remove;
using viua::assembler::backend::op_assemblers::assemble_op_float;
using viua::assembler::backend::op_assemblers::assemble_op_frame;

viua::internals::types::bytecode_size assemble_instruction(
    Program& program,
    viua::internals::types::bytecode_size& instruction,
    viua::internals::types::bytecode_size i,
    vector<Token> const& tokens,
    std::map<std::string, std::remove_reference<decltype(tokens)>::type::size_type>&
        marks) {
    /*  This is main assembly loop.
     *  It iterates over lines with instructions and
     *  uses bytecode generation API to fill the program with instructions and
     *  from them generate the bytecode.
     */
    log_location_being_assembled(tokens.at(i));

    if (tokens.at(i) == "nop") {
        program.opnop();
    } else if (tokens.at(i) == "izero") {
        assemble_single_register_op<&Program::opizero>(program, tokens, i);
    } else if (tokens.at(i) == "integer") {
        assemble_op_integer(program, tokens, i);
    } else if (tokens.at(i) == "iinc") {
        assemble_single_register_op<&Program::opiinc>(program, tokens, i);
    } else if (tokens.at(i) == "idec") {
        assemble_single_register_op<&Program::opidec>(program, tokens, i);
    } else if (tokens.at(i) == "float") {
        assemble_op_float(program, tokens, i);
    } else if (tokens.at(i) == "itof") {
        assemble_double_register_op<&Program::opitof>(program, tokens, i);
    } else if (tokens.at(i) == "ftoi") {
        assemble_double_register_op<&Program::opftoi>(program, tokens, i);
    } else if (tokens.at(i) == "stoi") {
        assemble_double_register_op<&Program::opstoi>(program, tokens, i);
    } else if (tokens.at(i) == "stof") {
        assemble_double_register_op<&Program::opstof>(program, tokens, i);
    } else if (tokens.at(i) == "add") {
        assemble_three_register_op<&Program::opadd>(program, tokens, i);
    } else if (tokens.at(i) == "sub") {
        assemble_three_register_op<&Program::opsub>(program, tokens, i);
    } else if (tokens.at(i) == "mul") {
        assemble_three_register_op<&Program::opmul>(program, tokens, i);
    } else if (tokens.at(i) == "div") {
        assemble_three_register_op<&Program::opdiv>(program, tokens, i);
    } else if (tokens.at(i) == "lt") {
        assemble_three_register_op<&Program::oplt>(program, tokens, i);
    } else if (tokens.at(i) == "lte") {
        assemble_three_register_op<&Program::oplte>(program, tokens, i);
    } else if (tokens.at(i) == "gt") {
        assemble_three_register_op<&Program::opgt>(program, tokens, i);
    } else if (tokens.at(i) == "gte") {
        assemble_three_register_op<&Program::opgte>(program, tokens, i);
    } else if (tokens.at(i) == "eq") {
        assemble_three_register_op<&Program::opeq>(program, tokens, i);
    } else if (tokens.at(i) == "string") {
        viua::assembler::backend::op_assemblers::assemble_op_string(program, tokens, i);
    } else if (tokens.at(i) == "text") {
        viua::assembler::backend::op_assemblers::assemble_op_text(program, tokens, i);
    } else if (tokens.at(i) == "texteq") {
        assemble_three_register_op<&Program::optexteq>(program, tokens, i);
    } else if (tokens.at(i) == "textat") {
        assemble_three_register_op<&Program::optextat>(program, tokens, i);
    } else if (tokens.at(i) == "textsub") {
        assemble_four_register_op<&Program::optextsub>(program, tokens, i);
    } else if (tokens.at(i) == "textlength") {
        assemble_double_register_op<&Program::optextlength>(program, tokens, i);
    } else if (tokens.at(i) == "textcommonprefix") {
        assemble_three_register_op<&Program::optextcommonprefix>(program, tokens, i);
    } else if (tokens.at(i) == "textcommonsuffix") {
        assemble_three_register_op<&Program::optextcommonsuffix>(program, tokens, i);
    } else if (tokens.at(i) == "textconcat") {
        assemble_three_register_op<&Program::optextconcat>(program, tokens, i);
    } else if (tokens.at(i) == "vector") {
        viua::assembler::backend::op_assemblers::assemble_op_vector(program, tokens, i);
    } else if (tokens.at(i) == "vinsert") {
        assemble_op_vinsert(program, tokens, i);
    } else if (tokens.at(i) == "vpush") {
        assemble_double_register_op<&Program::opvpush>(program, tokens, i);
    } else if (tokens.at(i) == "vpop") {
        assemble_op_vpop(program, tokens, i);
    } else if (tokens.at(i) == "vat") {
        assemble_three_register_op<&Program::opvat>(program, tokens, i);
    } else if (tokens.at(i) == "vlen") {
        assemble_double_register_op<&Program::opvlen>(program, tokens, i);
    } else if (tokens.at(i) == "not") {
        assemble_double_register_op<&Program::opnot>(program, tokens, i);
    } else if (tokens.at(i) == "and") {
        assemble_three_register_op<&Program::opand>(program, tokens, i);
    } else if (tokens.at(i) == "or") {
        assemble_three_register_op<&Program::opor>(program, tokens, i);
    } else if (tokens.at(i) == "bits") {
        assemble_op_bits(program, tokens, i);
    } else if (tokens.at(i) == "bitand") {
        assemble_three_register_op<&Program::opbitand>(program, tokens, i);
    } else if (tokens.at(i) == "bitor") {
        assemble_three_register_op<&Program::opbitor>(program, tokens, i);
    } else if (tokens.at(i) == "bitnot") {
        assemble_double_register_op<&Program::opnot>(program, tokens, i);
    } else if (tokens.at(i) == "bitxor") {
        assemble_three_register_op<&Program::opbitxor>(program, tokens, i);
    } else if (tokens.at(i) == "bitat") {
        assemble_three_register_op<&Program::opbitat>(program, tokens, i);
    } else if (tokens.at(i) == "bitset") {
        assemble_op_bitset(program, tokens, i);
    } else if (tokens.at(i) == "shl") {
        assemble_bit_shift_instruction<&Program::opshl>(program, tokens, i);
    } else if (tokens.at(i) == "ashl") {
        assemble_bit_shift_instruction<&Program::opashl>(program, tokens, i);
    } else if (tokens.at(i) == "shr") {
        assemble_bit_shift_instruction<&Program::opshr>(program, tokens, i);
    } else if (tokens.at(i) == "ashr") {
        assemble_bit_shift_instruction<&Program::opashr>(program, tokens, i);
    } else if (tokens.at(i) == "rol") {
        assemble_double_register_op<&Program::oprol>(program, tokens, i);
    } else if (tokens.at(i) == "ror") {
        assemble_double_register_op<&Program::opror>(program, tokens, i);
    } else if (tokens.at(i) == "wrapincrement") {
        assemble_increment_instruction<&Program::opwrapincrement>(
            program, tokens, i);
    } else if (tokens.at(i) == "wrapdecrement") {
        assemble_increment_instruction<&Program::opwrapdecrement>(
            program, tokens, i);
    } else if (tokens.at(i) == "wrapadd") {
        assemble_arithmetic_instruction<&Program::opwrapadd>(
            program, tokens, i);
    } else if (tokens.at(i) == "wrapsub") {
        assemble_arithmetic_instruction<&Program::opwrapsub>(
            program, tokens, i);
    } else if (tokens.at(i) == "wrapmul") {
        assemble_arithmetic_instruction<&Program::opwrapmul>(
            program, tokens, i);
    } else if (tokens.at(i) == "wrapdiv") {
        assemble_arithmetic_instruction<&Program::opwrapdiv>(
            program, tokens, i);
    } else if (tokens.at(i) == "checkedsincrement") {
        assemble_increment_instruction<&Program::opcheckedsincrement>(
            program, tokens, i);
    } else if (tokens.at(i) == "checkedsdecrement") {
        assemble_increment_instruction<&Program::opcheckedsdecrement>(
            program, tokens, i);
    } else if (tokens.at(i) == "checkedsadd") {
        assemble_arithmetic_instruction<&Program::opcheckedsadd>(
            program, tokens, i);
    } else if (tokens.at(i) == "checkedssub") {
        assemble_arithmetic_instruction<&Program::opcheckedssub>(
            program, tokens, i);
    } else if (tokens.at(i) == "checkedsmul") {
        assemble_arithmetic_instruction<&Program::opcheckedsmul>(
            program, tokens, i);
    } else if (tokens.at(i) == "checkedsdiv") {
        assemble_arithmetic_instruction<&Program::opcheckedsdiv>(
            program, tokens, i);
    } else if (tokens.at(i) == "checkeduincrement") {
        assemble_increment_instruction<&Program::opcheckeduincrement>(
            program, tokens, i);
    } else if (tokens.at(i) == "checkedudecrement") {
        assemble_increment_instruction<&Program::opcheckedudecrement>(
            program, tokens, i);
    } else if (tokens.at(i) == "checkeduadd") {
        assemble_arithmetic_instruction<&Program::opcheckeduadd>(
            program, tokens, i);
    } else if (tokens.at(i) == "checkedusub") {
        assemble_arithmetic_instruction<&Program::opcheckedusub>(
            program, tokens, i);
    } else if (tokens.at(i) == "checkedumul") {
        assemble_arithmetic_instruction<&Program::opcheckedumul>(
            program, tokens, i);
    } else if (tokens.at(i) == "checkedudiv") {
        assemble_arithmetic_instruction<&Program::opcheckedudiv>(
            program, tokens, i);
    } else if (tokens.at(i) == "saturatingsincrement") {
        assemble_increment_instruction<&Program::opsaturatingsincrement>(
            program, tokens, i);
    } else if (tokens.at(i) == "saturatingsdecrement") {
        assemble_increment_instruction<&Program::opsaturatingsdecrement>(
            program, tokens, i);
    } else if (tokens.at(i) == "saturatingsadd") {
        assemble_arithmetic_instruction<&Program::opsaturatingsadd>(
            program, tokens, i);
    } else if (tokens.at(i) == "saturatingssub") {
        assemble_arithmetic_instruction<&Program::opsaturatingssub>(
            program, tokens, i);
    } else if (tokens.at(i) == "saturatingsmul") {
        assemble_arithmetic_instruction<&Program::opsaturatingsmul>(
            program, tokens, i);
    } else if (tokens.at(i) == "saturatingsdiv") {
        assemble_arithmetic_instruction<&Program::opsaturatingsdiv>(
            program, tokens, i);
    } else if (tokens.at(i) == "saturatinguincrement") {
        assemble_increment_instruction<&Program::opsaturatinguincrement>(
            program, tokens, i);
    } else if (tokens.at(i) == "saturatingudecrement") {
        assemble_increment_instruction<&Program::opsaturatingudecrement>(
            program, tokens, i);
    } else if (tokens.at(i) == "saturatinguadd") {
        assemble_arithmetic_instruction<&Program::opsaturatinguadd>(
            program, tokens, i);
    } else if (tokens.at(i) == "saturatingusub") {
        assemble_arithmetic_instruction<&Program::opsaturatingusub>(
            program, tokens, i);
    } else if (tokens.at(i) == "saturatingumul") {
        assemble_arithmetic_instruction<&Program::opsaturatingumul>(
            program, tokens, i);
    } else if (tokens.at(i) == "saturatingudiv") {
        assemble_arithmetic_instruction<&Program::opsaturatingudiv>(
            program, tokens, i);
    } else if (tokens.at(i) == "move") {
        assemble_double_register_op<&Program::opmove>(program, tokens, i);
    } else if (tokens.at(i) == "copy") {
        assemble_double_register_op<&Program::opcopy>(program, tokens, i);
    } else if (tokens.at(i) == "ptr") {
        assemble_double_register_op<&Program::opptr>(program, tokens, i);
    } else if (tokens.at(i) == "swap") {
        assemble_double_register_op<&Program::opswap>(program, tokens, i);
    } else if (tokens.at(i) == "delete") {
        assemble_single_register_op<&Program::opdelete>(program, tokens, i);
    } else if (tokens.at(i) == "isnull") {
        assemble_double_register_op<&Program::opisnull>(program, tokens, i);
    } else if (tokens.at(i) == "print") {
        assemble_single_register_op<&Program::opprint>(program, tokens, i);
    } else if (tokens.at(i) == "echo") {
        assemble_single_register_op<&Program::opecho>(program, tokens, i);
    } else if (tokens.at(i) == "capture") {
        assemble_capture_op<&Program::opcapture>(program, tokens, i);
    } else if (tokens.at(i) == "capturecopy") {
        assemble_capture_op<&Program::opcapturecopy>(program, tokens, i);
    } else if (tokens.at(i) == "capturemove") {
        assemble_capture_op<&Program::opcapturemove>(program, tokens, i);
    } else if (tokens.at(i) == "closure") {
        assemble_fn_ctor_op<&Program::opclosure>(program, tokens, i);
    } else if (tokens.at(i) == "function") {
        assemble_fn_ctor_op<&Program::opfunction>(program, tokens, i);
    } else if (tokens.at(i) == "frame") {
        assemble_op_frame(program, tokens, i);
    } else if (tokens.at(i) == "param") {
        Token_index target = i + 1;
        Token_index source = target + 1;

        program.opparam(
            assembler::operands::getint(::assembler::operands::resolve_register(tokens.at(target))),
            assembler::operands::getint_with_rs_type(
                ::assembler::operands::resolve_register(tokens.at(source)),
                ::assembler::operands::resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "pamv") {
        Token_index target = i + 1;
        Token_index source = target + 1;

        program.oppamv(
            assembler::operands::getint(::assembler::operands::resolve_register(tokens.at(target))),
            assembler::operands::getint_with_rs_type(
                ::assembler::operands::resolve_register(tokens.at(source)),
                ::assembler::operands::resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "arg") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        if (tokens.at(target) == "void") {
            --source;
            program.oparg(
                assembler::operands::getint(::assembler::operands::resolve_register(tokens.at(target))),
                assembler::operands::getint(
                    ::assembler::operands::resolve_register(tokens.at(source))));
        } else {
            program.oparg(assembler::operands::getint_with_rs_type(
                              ::assembler::operands::resolve_register(tokens.at(target)),
                              ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                          assembler::operands::getint(
                              ::assembler::operands::resolve_register(tokens.at(source))));
        }
    } else if (tokens.at(i) == "argc") {
        Token_index target = i + 1;

        program.opargc(assembler::operands::getint_with_rs_type(
            ::assembler::operands::resolve_register(tokens.at(target)),
            ::assembler::operands::resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "call") {
        assemble_op_call(program, tokens, i);
    } else if (tokens.at(i) == "tailcall") {
        if (tokens.at(i + 1).str().at(0) == '*'
            or tokens.at(i + 1).str().at(0) == '%') {
            program.optailcall(assembler::operands::getint_with_rs_type(
                ::assembler::operands::resolve_register(tokens.at(i + 1)),
                ::assembler::operands::resolve_rs_type(tokens.at(i + 2))));
        } else {
            program.optailcall(tokens.at(i + 1));
        }
    } else if (tokens.at(i) == "defer") {
        if (tokens.at(i + 1).str().at(0) == '*'
            or tokens.at(i + 1).str().at(0) == '%') {
            program.opdefer(assembler::operands::getint_with_rs_type(
                ::assembler::operands::resolve_register(tokens.at(i + 1)),
                ::assembler::operands::resolve_rs_type(tokens.at(i + 2))));
        } else {
            program.opdefer(tokens.at(i + 1));
        }
    } else if (tokens.at(i) == "process") {
        Token_index target = i + 1;
        Token_index fn     = target + 2;

        int_op ret;
        if (tokens.at(target) == "void") {
            --fn;
            ret =
                assembler::operands::getint(::assembler::operands::resolve_register(tokens.at(target)));
        } else {
            ret = assembler::operands::getint_with_rs_type(
                ::assembler::operands::resolve_register(tokens.at(target)),
                ::assembler::operands::resolve_rs_type(tokens.at(target + 1)));
        }

        program.opprocess(ret, tokens.at(fn));
    } else if (tokens.at(i) == "self") {
        Token_index target = i + 1;

        program.opself(assembler::operands::getint_with_rs_type(
            ::assembler::operands::resolve_register(tokens.at(target)),
            ::assembler::operands::resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "join") {
        Token_index target        = i + 1;
        Token_index process       = target + 2;
        Token_index timeout_index = process + 2;

        int_op target_operand;
        if (tokens.at(target) == "void") {
            --process;
            --timeout_index;
            target_operand =
                assembler::operands::getint(::assembler::operands::resolve_register(tokens.at(target)));
        } else {
            target_operand = assembler::operands::getint_with_rs_type(
                ::assembler::operands::resolve_register(tokens.at(target)),
                ::assembler::operands::resolve_rs_type(tokens.at(target + 1)));
        }

        timeout_op timeout =
            convert_token_to_timeout_operand(tokens.at(timeout_index));
        program.opjoin(target_operand,
                       assembler::operands::getint_with_rs_type(
                           ::assembler::operands::resolve_register(tokens.at(process)),
                           ::assembler::operands::resolve_rs_type(tokens.at(process + 1))),
                       timeout);
    } else if (tokens.at(i) == "send") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opsend(assembler::operands::getint_with_rs_type(
                           ::assembler::operands::resolve_register(tokens.at(target)),
                           ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                       assembler::operands::getint_with_rs_type(
                           ::assembler::operands::resolve_register(tokens.at(source)),
                           ::assembler::operands::resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "receive") {
        Token_index target        = i + 1;
        Token_index timeout_index = target + 2;

        int_op target_operand;
        if (tokens.at(target) == "void") {
            --timeout_index;
            target_operand =
                assembler::operands::getint(::assembler::operands::resolve_register(tokens.at(target)));
        } else {
            target_operand = assembler::operands::getint_with_rs_type(
                ::assembler::operands::resolve_register(tokens.at(target)),
                ::assembler::operands::resolve_rs_type(tokens.at(target + 1)));
        }

        timeout_op timeout =
            convert_token_to_timeout_operand(tokens.at(timeout_index));

        program.opreceive(target_operand, timeout);
    } else if (tokens.at(i) == "watchdog") {
        program.opwatchdog(tokens.at(i + 1));
    } else if (tokens.at(i) == "if") {
        assemble_op_if(program, tokens, i, instruction, marks);
    } else if (tokens.at(i) == "jump") {
        assemble_op_jump(program, tokens, i, instruction, marks);
    } else if (tokens.at(i) == "try") {
        program.optry();
    } else if (tokens.at(i) == "catch") {
        string type_chnk, catcher_chnk;
        program.opcatch(tokens.at(i + 1), tokens.at(i + 2));
    } else if (tokens.at(i) == "draw") {
        Token_index target = i + 1;

        program.opdraw(assembler::operands::getint_with_rs_type(
            ::assembler::operands::resolve_register(tokens.at(target)),
            ::assembler::operands::resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "enter") {
        program.openter(tokens.at(i + 1));
    } else if (tokens.at(i) == "throw") {
        Token_index source = i + 1;

        program.opthrow(assembler::operands::getint_with_rs_type(
            ::assembler::operands::resolve_register(tokens.at(source)),
            ::assembler::operands::resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "leave") {
        program.opleave();
    } else if (tokens.at(i) == "import") {
        program.opimport(tokens.at(i + 1));
    } else if (tokens.at(i) == "class") {
        Token_index target     = i + 1;
        Token_index class_name = target + 2;

        program.opclass(assembler::operands::getint_with_rs_type(
                            ::assembler::operands::resolve_register(tokens.at(target)),
                            ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                        tokens.at(class_name));
    } else if (tokens.at(i) == "derive") {
        Token_index target     = i + 1;
        Token_index class_name = target + 2;

        program.opderive(assembler::operands::getint_with_rs_type(
                             ::assembler::operands::resolve_register(tokens.at(target)),
                             ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                         tokens.at(class_name));
    } else if (tokens.at(i) == "attach") {
        Token_index target        = i + 1;
        Token_index fn_name       = target + 2;
        Token_index attached_name = fn_name + 1;

        program.opattach(assembler::operands::getint_with_rs_type(
                             ::assembler::operands::resolve_register(tokens.at(target)),
                             ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                         tokens.at(fn_name),
                         tokens.at(attached_name));
    } else if (tokens.at(i) == "register") {
        Token_index target = i + 1;

        program.opregister(assembler::operands::getint_with_rs_type(
            ::assembler::operands::resolve_register(tokens.at(target)),
            ::assembler::operands::resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "atom") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opatom(assembler::operands::getint_with_rs_type(
                           ::assembler::operands::resolve_register(tokens.at(target)),
                           ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                       tokens.at(source));
    } else if (tokens.at(i) == "atomeq") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opatomeq(assembler::operands::getint_with_rs_type(
                             ::assembler::operands::resolve_register(tokens.at(target)),
                             ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                         assembler::operands::getint_with_rs_type(
                             ::assembler::operands::resolve_register(tokens.at(lhs)),
                             ::assembler::operands::resolve_rs_type(tokens.at(lhs + 1))),
                         assembler::operands::getint_with_rs_type(
                             ::assembler::operands::resolve_register(tokens.at(rhs)),
                             ::assembler::operands::resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "struct") {
        Token_index target = i + 1;

        program.opstruct(assembler::operands::getint_with_rs_type(
            ::assembler::operands::resolve_register(tokens.at(target)),
            ::assembler::operands::resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "structinsert") {
        Token_index target = i + 1;
        Token_index key    = target + 2;
        Token_index source = key + 2;

        program.opstructinsert(assembler::operands::getint_with_rs_type(
                                   ::assembler::operands::resolve_register(tokens.at(target)),
                                   ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                               assembler::operands::getint_with_rs_type(
                                   ::assembler::operands::resolve_register(tokens.at(key)),
                                   ::assembler::operands::resolve_rs_type(tokens.at(key + 1))),
                               assembler::operands::getint_with_rs_type(
                                   ::assembler::operands::resolve_register(tokens.at(source)),
                                   ::assembler::operands::resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "structremove") {
        assemble_op_structremove(program, tokens, i);
    } else if (tokens.at(i) == "structkeys") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opstructkeys(assembler::operands::getint_with_rs_type(
                                 ::assembler::operands::resolve_register(tokens.at(target)),
                                 ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                             assembler::operands::getint_with_rs_type(
                                 ::assembler::operands::resolve_register(tokens.at(source)),
                                 ::assembler::operands::resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "new") {
        Token_index target     = i + 1;
        Token_index class_name = target + 2;

        program.opnew(assembler::operands::getint_with_rs_type(
                          ::assembler::operands::resolve_register(tokens.at(target)),
                          ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                      tokens.at(class_name));
    } else if (tokens.at(i) == "msg") {
        assemble_op_msg(program, tokens, i);
    } else if (tokens.at(i) == "insert") {
        Token_index target = i + 1;
        Token_index key    = target + 2;
        Token_index source = key + 2;

        program.opinsert(assembler::operands::getint_with_rs_type(
                             ::assembler::operands::resolve_register(tokens.at(target)),
                             ::assembler::operands::resolve_rs_type(tokens.at(target + 1))),
                         assembler::operands::getint_with_rs_type(
                             ::assembler::operands::resolve_register(tokens.at(key)),
                             ::assembler::operands::resolve_rs_type(tokens.at(key + 1))),
                         assembler::operands::getint_with_rs_type(
                             ::assembler::operands::resolve_register(tokens.at(source)),
                             ::assembler::operands::resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "remove") {
        assemble_op_remove(program, tokens, i);
    } else if (tokens.at(i) == "return") {
        program.opreturn();
    } else if (tokens.at(i) == "halt") {
        program.ophalt();
    } else if (tokens.at(i).str().substr(0, 1) == ".") {
        // do nothing, it's an assembler directive
    } else {
        throw viua::cg::lex::InvalidSyntax(
            tokens.at(i),
            ("unimplemented instruction: "
             + str::enquote(str::strencode(tokens.at(i)))));
    }

    if (tokens.at(i).str().substr(0, 1) != ".") {
        ++instruction;
    }

    while (tokens.at(++i) != "\n")
        ;
    ++i;  // skip the newline
    return i;
}
