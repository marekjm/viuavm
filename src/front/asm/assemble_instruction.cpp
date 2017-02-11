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

#include <cstdint>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <viua/machine.h>
#include <viua/bytecode/maps.h>
#include <viua/support/string.h>
#include <viua/support/env.h>
#include <viua/loader.h>
#include <viua/program.h>
#include <viua/cg/tools.h>
#include <viua/cg/tokenizer.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/front/asm.h>
using namespace std;


extern bool DEBUG;
extern bool SCREAM;


using Token = viua::cg::lex::Token;


static tuple<viua::internals::types::bytecode_size, enum JUMPTYPE> resolvejump(Token token, const map<string, vector<Token>::size_type>& marks, viua::internals::types::bytecode_size instruction_index) {
    /*  This function is used to resolve jumps in `jump` and `branch` instructions.
     */
    string jmp = token.str();
    viua::internals::types::bytecode_size addr = 0;
    enum JUMPTYPE jump_type = JMP_RELATIVE;
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
            // FIXME: move jump verification to assembler::verify namespace function
            ostringstream oss;
            oss << "use of relative jump results in a jump to negative index: ";
            oss << "jump_value = " << jump_value << ", ";
            oss << "instruction_index = " << instruction_index;
            throw viua::cg::lex::InvalidSyntax(token, oss.str());
        }
        addr = (instruction_index - static_cast<viua::internals::types::bytecode_size>(-1 * jump_value));
    } else if (jmp[0] == '+') {
        addr = (instruction_index + stoul(jmp.substr(1)));
    } else {
        try {
            // FIXME: markers map should use viua::internals::types::bytecode_size to avoid the need for casting
            addr = static_cast<viua::internals::types::bytecode_size>(marks.at(jmp));
        } catch (const std::out_of_range& e) {
            throw viua::cg::lex::InvalidSyntax(token, ("cannot resolve jump to unrecognised marker: " + str::enquote(str::strencode(jmp))));
        }
    }

    // FIXME: check if the jump is within the size of bytecode
    return tuple<viua::internals::types::bytecode_size, enum JUMPTYPE>(addr, jump_type);
}

static string resolveregister(Token token, const bool allow_bare_integers = false) {
    /*  This function is used to register numbers when a register is accessed, e.g.
     *  in `istore` instruction or in `branch` in condition operand.
     *
     *  This function MUST return string as teh result is further passed to assembler::operands::getint() function which *expects* string.
     */
    ostringstream out;
    string reg = token.str();
    if (reg[0] == '@' and str::isnum(str::sub(reg, 1))) {
        /*  Basic case - the register index is taken from another register, everything is still nice and simple.
         */
        if (stoi(reg.substr(1)) < 0) {
            throw ("register indexes cannot be negative: " + reg);
        }

        // FIXME: analyse source and detect if the referenced register really holds an integer (the only value suitable to use
        // as register reference)
        out.str(reg);
    } else if (reg[0] == '*' and str::isnum(str::sub(reg, 1))) {
        /*  Basic case - the register index is taken from another register, everything is still nice and simple.
         */
        if (stoi(reg.substr(1)) < 0) {
            throw ("register indexes cannot be negative: " + reg);
        }

        out.str(reg);
    } else if (reg[0] == '%' and str::isnum(str::sub(reg, 1))) {
        /*  Basic case - the register index is taken from another register, everything is still nice and simple.
         */
        if (stoi(reg.substr(1)) < 0) {
            throw ("register indexes cannot be negative: " + reg);
        }

        out.str(reg);
    } else if (reg == "void") {
        out << reg;
    } else if (allow_bare_integers and str::isnum(reg)) {
        out << reg;
    } else {
        throw viua::cg::lex::InvalidSyntax(token, ("cannot resolve register operand: " + str::enquote(str::strencode(token.str()))));
    }
    return out.str();
}

static viua::internals::RegisterSets resolve_rs_type(Token token) {
    if (token == "current") {
        return viua::internals::RegisterSets::CURRENT;
    } else if (token == "local") {
        return viua::internals::RegisterSets::LOCAL;
    } else if (token == "static") {
        return viua::internals::RegisterSets::STATIC;
    } else if (token == "global") {
        return viua::internals::RegisterSets::GLOBAL;
    } else {
        throw viua::cg::lex::InvalidSyntax(token, "invalid register set type name");
    }
}

static viua::internals::types::timeout timeout_to_int(const string& timeout) {
    const auto timeout_str_size = timeout.size();
    if (timeout[timeout_str_size-2] == 'm') {
        return static_cast<viua::internals::types::timeout>(stoi(timeout.substr(0, timeout_str_size-2)));
    } else {
        return static_cast<viua::internals::types::timeout>((stoi(timeout.substr(0, timeout_str_size-1)) * 1000));
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
static auto convert_token_to_timeout_operand(viua::cg::lex::Token token) -> timeout_op {
    viua::internals::types::timeout timeout_milliseconds = 0;
    if (token != "infinity") {
        timeout_milliseconds = timeout_to_int(token);
        ++timeout_milliseconds;
    }
    return timeout_op{timeout_milliseconds};
}
viua::internals::types::bytecode_size assemble_instruction(Program& program, viua::internals::types::bytecode_size& instruction, viua::internals::types::bytecode_size i, const vector<Token>& tokens, map<string, std::remove_reference<decltype(tokens)>::type::size_type>& marks) {
    /*  This is main assembly loop.
     *  It iterates over lines with instructions and
     *  uses bytecode generation API to fill the program with instructions and
     *  from them generate the bytecode.
     */
    if (DEBUG and SCREAM) {
        cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << "message" << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "assembling '";
        cout << send_control_seq(COLOR_FG_WHITE) << tokens.at(i).str() << send_control_seq(ATTR_RESET);
        cout << "' instruction\n";
    }

    using TokenIndex = std::remove_reference<decltype(tokens)>::type::size_type;

    if (tokens.at(i) == "nop") {
        program.opnop();
    } else if (tokens.at(i) == "izero") {
        TokenIndex target = i + 1;

        program.opizero(assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1))));
    } else if (tokens.at(i) == "istore") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opistore(assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1))), assembler::operands::getint(resolveregister(tokens.at(source), true), true));
    } else if (tokens.at(i) == "iinc") {
        TokenIndex target = i + 1;

        program.opiinc(assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1))));
    } else if (tokens.at(i) == "idec") {
        TokenIndex target = i + 1;

        program.opidec(assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1))));
    } else if (tokens.at(i) == "fstore") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opfstore(assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1))), stod(tokens.at(source).str()));
    } else if (tokens.at(i) == "itof") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opitof(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "ftoi") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opftoi(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "stoi") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opstoi(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "stof") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opstof(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "add") {
        TokenIndex result_type = i + 1;
        TokenIndex target = result_type + 1;
        TokenIndex lhs = target + 2;
        TokenIndex rhs = lhs + 2;

        program.opadd(
            tokens.at(result_type)
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(lhs)), resolve_rs_type(tokens.at(lhs+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(rhs)), resolve_rs_type(tokens.at(rhs+1)))
        );
    } else if (tokens.at(i) == "sub") {
        TokenIndex result_type = i + 1;
        TokenIndex target = result_type + 1;
        TokenIndex lhs = target + 2;
        TokenIndex rhs = lhs + 2;

        program.opsub(
            tokens.at(result_type)
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(lhs)), resolve_rs_type(tokens.at(lhs+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(rhs)), resolve_rs_type(tokens.at(rhs+1)))
        );
    } else if (tokens.at(i) == "mul") {
        TokenIndex result_type = i + 1;
        TokenIndex target = result_type + 1;
        TokenIndex lhs = target + 2;
        TokenIndex rhs = lhs + 2;

        program.opmul(
            tokens.at(result_type)
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(lhs)), resolve_rs_type(tokens.at(lhs+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(rhs)), resolve_rs_type(tokens.at(rhs+1)))
        );
    } else if (tokens.at(i) == "div") {
        TokenIndex result_type = i + 1;
        TokenIndex target = result_type + 1;
        TokenIndex lhs = target + 2;
        TokenIndex rhs = lhs + 2;

        program.opdiv(
            tokens.at(result_type)
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(lhs)), resolve_rs_type(tokens.at(lhs+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(rhs)), resolve_rs_type(tokens.at(rhs+1)))
        );
    } else if (tokens.at(i) == "lt") {
        TokenIndex result_type = i + 1;
        TokenIndex target = result_type + 1;
        TokenIndex lhs = target + 2;
        TokenIndex rhs = lhs + 2;

        program.oplt(
            tokens.at(result_type)
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(lhs)), resolve_rs_type(tokens.at(lhs+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(rhs)), resolve_rs_type(tokens.at(rhs+1)))
        );
    } else if (tokens.at(i) == "lte") {
        TokenIndex result_type = i + 1;
        TokenIndex target = result_type + 1;
        TokenIndex lhs = target + 2;
        TokenIndex rhs = lhs + 2;

        program.oplte(
            tokens.at(result_type)
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(lhs)), resolve_rs_type(tokens.at(lhs+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(rhs)), resolve_rs_type(tokens.at(rhs+1)))
        );
    } else if (tokens.at(i) == "gt") {
        TokenIndex result_type = i + 1;
        TokenIndex target = result_type + 1;
        TokenIndex lhs = target + 2;
        TokenIndex rhs = lhs + 2;

        program.opgt(
            tokens.at(result_type)
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(lhs)), resolve_rs_type(tokens.at(lhs+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(rhs)), resolve_rs_type(tokens.at(rhs+1)))
        );
    } else if (tokens.at(i) == "gte") {
        TokenIndex result_type = i + 1;
        TokenIndex target = result_type + 1;
        TokenIndex lhs = target + 2;
        TokenIndex rhs = lhs + 2;

        program.opgte(
            tokens.at(result_type)
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(lhs)), resolve_rs_type(tokens.at(lhs+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(rhs)), resolve_rs_type(tokens.at(rhs+1)))
        );
    } else if (tokens.at(i) == "eq") {
        TokenIndex result_type = i + 1;
        TokenIndex target = result_type + 1;
        TokenIndex lhs = target + 2;
        TokenIndex rhs = lhs + 2;

        program.opeq(
            tokens.at(result_type)
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(lhs)), resolve_rs_type(tokens.at(lhs+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(rhs)), resolve_rs_type(tokens.at(rhs+1)))
        );
    } else if (tokens.at(i) == "strstore") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opstrstore(assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1))), tokens.at(source));
    } else if (tokens.at(i) == "vec") {
        TokenIndex target = i + 1;
        TokenIndex pack_range_start = target + 2;
        TokenIndex pack_range_count = pack_range_start + 2;

        program.opvec(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(pack_range_start)), resolve_rs_type(tokens.at(pack_range_start+1)))
            , assembler::operands::getint(resolveregister(tokens.at(pack_range_count)))
        );
    } else if (tokens.at(i) == "vinsert") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;
        TokenIndex position = source + 2;

        program.opvinsert(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
            , assembler::operands::getint(resolveregister(tokens.at(position)))
        );
    } else if (tokens.at(i) == "vpush") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opvpush(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "vpop") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;
        TokenIndex position = source + 2;

        if (tokens.at(target) == "void") {
            --source;
            --position;
            program.opvpop(
                assembler::operands::getint(resolveregister(tokens.at(target)))
                , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
                , assembler::operands::getint(resolveregister(tokens.at(position), true), true)
            );
        } else {
            program.opvpop(
                assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
                , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
                , assembler::operands::getint(resolveregister(tokens.at(position), true), true)
            );
        }
    } else if (tokens.at(i) == "vat") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;
        TokenIndex position = source + 2;

        program.opvat(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
            , assembler::operands::getint(resolveregister(tokens.at(position), true), true)
        );
    } else if (tokens.at(i) == "vlen") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opvlen(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "not") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opnot(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "and") {
        TokenIndex target = i + 1;
        TokenIndex lhs = target + 2;
        TokenIndex rhs = lhs + 2;

        program.opand(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(lhs)), resolve_rs_type(tokens.at(lhs+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(rhs)), resolve_rs_type(tokens.at(rhs+1)))
        );
    } else if (tokens.at(i) == "or") {
        TokenIndex target = i + 1;
        TokenIndex lhs = target + 2;
        TokenIndex rhs = lhs + 2;

        program.opor(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(lhs)), resolve_rs_type(tokens.at(lhs+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(rhs)), resolve_rs_type(tokens.at(rhs+1)))
        );
    } else if (tokens.at(i) == "move") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opmove(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "copy") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opcopy(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "ptr") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opptr(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "swap") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opswap(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "delete") {
        TokenIndex target = i + 1;

        program.opdelete(assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1))));
    } else if (tokens.at(i) == "isnull") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opisnull(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "ress") {
        TokenIndex target = i + 1;

        program.opress(tokens.at(target));
    } else if (tokens.at(i) == "print") {
        TokenIndex source = i + 1;

        program.opprint(assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1))));
    } else if (tokens.at(i) == "echo") {
        TokenIndex source = i + 1;

        program.opecho(assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1))));
    } else if (tokens.at(i) == "capture") {
        TokenIndex target = i + 1;
        TokenIndex inside_index = target + 2;
        TokenIndex source = inside_index + 1;

        program.opcapture(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint(resolveregister(tokens.at(inside_index)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "capturecopy") {
        TokenIndex target = i + 1;
        TokenIndex inside_index = target + 2;
        TokenIndex source = inside_index + 1;

        program.opcapturecopy(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint(resolveregister(tokens.at(inside_index)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "capturemove") {
        TokenIndex target = i + 1;
        TokenIndex inside_index = target + 2;
        TokenIndex source = inside_index + 1;

        program.opcapturemove(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint(resolveregister(tokens.at(inside_index)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "closure") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opclosure(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , tokens.at(source)
        );
    } else if (tokens.at(i) == "function") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opfunction(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , tokens.at(source)
        );
    } else if (tokens.at(i) == "fcall") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        if (tokens.at(target) == "void") {
            --source;
            program.opfcall(
                assembler::operands::getint(resolveregister(tokens.at(target)))
                , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
            );
        } else {
            program.opfcall(
                assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
                , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
            );
        }
    } else if (tokens.at(i) == "frame") {
        TokenIndex target = get_token_index_of_operand(tokens, i, 1);
        TokenIndex source = get_token_index_of_operand(tokens, i, 2);

        program.opframe(assembler::operands::getint(resolveregister(tokens.at(target))), assembler::operands::getint(resolveregister(tokens.at(source))));
    } else if (tokens.at(i) == "param") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 1;

        program.opparam(
            assembler::operands::getint(resolveregister(tokens.at(target)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "pamv") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 1;

        program.oppamv(
            assembler::operands::getint(resolveregister(tokens.at(target)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "arg") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        if (tokens.at(target) == "void") {
            --source;
            program.oparg(
                assembler::operands::getint(resolveregister(tokens.at(target)))
                , assembler::operands::getint(resolveregister(tokens.at(source)))
            );
        } else {
            program.oparg(
                assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
                , assembler::operands::getint(resolveregister(tokens.at(source)))
            );
        }
    } else if (tokens.at(i) == "argc") {
        TokenIndex target = i + 1;

        program.opargc(assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1))));
    } else if (tokens.at(i) == "call") {
        /** Full form of call instruction has two operands: function name and
         *  return value register index.
         *  If call is given only one operand it means it is the function name and
         *  returned value is discarded.
         *  To explicitly state that return value should be discarderd put `void` as
         *  return register index.
         */
        /** Why is the function supplied as a *string* and
         *  not direct instruction pointer?
         *  That would be faster - c'mon couldn't assembler just calculate offsets and
         *  insert them?
         *
         *  Nope.
         *
         *  Yes, it *would* be faster if calls were just precalculated jumps.
         *  However, by them being strings we get plenty of flexibility, good-quality
         *  stack traces, and
         *  a place to put plenty of debugging info.
         *  All that at a cost of just one map lookup; the overhead is minimal and
         *  gains are big.
         *  What's not to love?
         *
         *  Of course, you, my dear reader, are free to take this code (it's GPL after
         *  all!) and modify it to suit your particular needs - in that case that would
         *  be calculating call jumps at compile time and exchanging CALL instructions
         *  with JUMP instructions.
         *
         *  Good luck with debugging your code, then.
         */
        TokenIndex target = i + 1;
        TokenIndex fn = target + 2;

        int_op ret;
        if (tokens.at(target) == "void") {
            --fn;
            ret = assembler::operands::getint(resolveregister(tokens.at(target)));
        } else {
            ret = assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(target))
                , resolve_rs_type(tokens.at(target+1))
            );
        }

        program.opcall(ret, tokens.at(fn));
    } else if (tokens.at(i) == "tailcall") {
        program.optailcall(tokens.at(i+1));
    } else if (tokens.at(i) == "process") {
        TokenIndex target = i + 1;
        TokenIndex fn = target + 2;

        int_op ret;
        if (tokens.at(target) == "void") {
            --fn;
            ret = assembler::operands::getint(resolveregister(tokens.at(target)));
        } else {
            ret = assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(target))
                , resolve_rs_type(tokens.at(target+1))
            );
        }

        program.opprocess(ret, tokens.at(fn));
    } else if (tokens.at(i) == "self") {
        TokenIndex target = i + 1;

        program.opself(assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1))));
    } else if (tokens.at(i) == "join") {
        TokenIndex target = i + 1;
        TokenIndex process = target + 2;
        TokenIndex timeout_index = process + 2;

        int_op target_operand;
        if (tokens.at(target) == "void") {
            --process;
            --timeout_index;
            target_operand = assembler::operands::getint(resolveregister(tokens.at(target)));
        } else {
            target_operand = assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(target))
                , resolve_rs_type(tokens.at(target+1))
            );
        }

        timeout_op timeout = convert_token_to_timeout_operand(tokens.at(timeout_index));
        program.opjoin(
            target_operand
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(process)), resolve_rs_type(tokens.at(process+1)))
            , timeout
        );
    } else if (tokens.at(i) == "send") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;

        program.opsend(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "receive") {
        TokenIndex target = i + 1;
        TokenIndex timeout_index = target + 2;

        int_op target_operand;
        if (tokens.at(target) == "void") {
            --timeout_index;
            target_operand = assembler::operands::getint(resolveregister(tokens.at(target)));
        } else {
            target_operand = assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(target))
                , resolve_rs_type(tokens.at(target+1))
            );
        }

        timeout_op timeout = convert_token_to_timeout_operand(tokens.at(timeout_index));

        program.opreceive(target_operand, timeout);
    } else if (tokens.at(i) == "watchdog") {
        program.opwatchdog(tokens.at(i+1));
    } else if (tokens.at(i) == "if") {
        /*  If branch is given three operands, it means its full, three-operands form is being used.
         *  Otherwise, it is short, two-operands form instruction and assembler should fill third operand accordingly.
         *
         *  In case of short-form `branch` instruction:
         *
         *      * first operand is index of the register to check,
         *      * second operand is the address to which to jump if register is true,
         *      * third operand is assumed to be the *next instruction*, i.e. instruction after the branch instruction,
         *
         *  In full (with three operands) form of `branch` instruction:
         *
         *      * third operands is the address to which to jump if register is false,
         */
        // FIXME jump indexes
        Token condition = tokens.at(i+1), if_true = tokens.at(i+2), if_false = tokens.at(i+3);

        viua::internals::types::bytecode_size addrt_target, addrf_target;
        enum JUMPTYPE addrt_jump_type, addrf_jump_type;
        tie(addrt_target, addrt_jump_type) = resolvejump(tokens.at(i+2), marks, instruction);
        if (if_false != "\n") {
            tie(addrf_target, addrf_jump_type) = resolvejump(tokens.at(i+3), marks, instruction);
        } else {
            addrf_jump_type = JMP_RELATIVE;
            addrf_target = instruction+1;
        }

        program.opif(assembler::operands::getint(resolveregister(condition)), addrt_target, addrt_jump_type, addrf_target, addrf_jump_type);
    } else if (tokens.at(i) == "jump") {
        /*  Jump instruction can be written in two forms:
         *
         *      * `jump <index>`
         *      * `jump :<marker>`
         *
         *  Assembler must distinguish between these two forms, and so it does.
         *  Here, we use a function from string support lib to determine
         *  if the jump is numeric, and thus an index, or
         *  a string - in which case we consider it a marker jump.
         *
         *  If it is a marker jump, assembler will look the marker up in a map and
         *  if it is not found throw an exception about unrecognised marker being used.
         */
        viua::internals::types::bytecode_size jump_target;
        enum JUMPTYPE jump_type;
        tie(jump_target, jump_type) = resolvejump(tokens.at(i+1), marks, instruction);

        program.opjump(jump_target, jump_type);
    } else if (tokens.at(i) == "try") {
        program.optry();
    } else if (tokens.at(i) == "catch") {
        string type_chnk, catcher_chnk;
        program.opcatch(tokens.at(i+1), tokens.at(i+2));
    } else if (tokens.at(i) == "draw") {
        TokenIndex target = i + 1;

        program.opdraw(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
        );
    } else if (tokens.at(i) == "enter") {
        program.openter(tokens.at(i+1));
    } else if (tokens.at(i) == "throw") {
        TokenIndex source = i + 1;

        program.opthrow(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "leave") {
        program.opleave();
    } else if (tokens.at(i) == "import") {
        program.opimport(tokens.at(i+1));
    } else if (tokens.at(i) == "link") {
        program.oplink(tokens.at(i+1));
    } else if (tokens.at(i) == "class") {
        TokenIndex target = i + 1;
        TokenIndex class_name = target + 2;

        program.opclass(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , tokens.at(class_name)
        );
    } else if (tokens.at(i) == "derive") {
        TokenIndex target = i + 1;
        TokenIndex class_name = target + 2;

        program.opderive(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , tokens.at(class_name)
        );
    } else if (tokens.at(i) == "attach") {
        TokenIndex target = i + 1;
        TokenIndex fn_name = target + 2;
        TokenIndex attached_name = fn_name+1;

        program.opattach(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , tokens.at(fn_name)
            , tokens.at(attached_name)
        );
    } else if (tokens.at(i) == "register") {
        TokenIndex target = i + 1;

        program.opregister(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
        );
    } else if (tokens.at(i) == "new") {
        TokenIndex target = i + 1;
        TokenIndex class_name = target + 2;

        program.opnew(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , tokens.at(class_name)
        );
    } else if (tokens.at(i) == "msg") {
        TokenIndex target = i + 1;
        TokenIndex fn = target + 2;

        int_op ret;
        if (tokens.at(target) == "void") {
            --fn;
            ret = assembler::operands::getint(resolveregister(tokens.at(target)));
        } else {
            ret = assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(target))
                , resolve_rs_type(tokens.at(target+1))
            );
        }

        program.opmsg(ret, tokens.at(fn));
    } else if (tokens.at(i) == "insert") {
        TokenIndex target = i + 1;
        TokenIndex key = target + 2;
        TokenIndex source = key + 2;

        program.opinsert(
            assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(key)), resolve_rs_type(tokens.at(key+1)))
            , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
        );
    } else if (tokens.at(i) == "remove") {
        TokenIndex target = i + 1;
        TokenIndex source = target + 2;
        TokenIndex key = source + 2;

        if (tokens.at(target) == "void") {
            --source;
            --key;
            program.opremove(
                assembler::operands::getint(resolveregister(tokens.at(target)))
                , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
                , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(key)), resolve_rs_type(tokens.at(key+1)))
            );
        } else {
            program.opremove(
                assembler::operands::getint_with_rs_type(resolveregister(tokens.at(target)), resolve_rs_type(tokens.at(target+1)))
                , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(source)), resolve_rs_type(tokens.at(source+1)))
                , assembler::operands::getint_with_rs_type(resolveregister(tokens.at(key)), resolve_rs_type(tokens.at(key+1)))
            );
        }
    } else if (tokens.at(i) == "return") {
        program.opreturn();
    } else if (tokens.at(i) == "halt") {
        program.ophalt();
    } else if (tokens.at(i).str().substr(0, 1) == ".") {
        // do nothing, it's an assembler directive
    } else {
        throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("unimplemented instruction: " + str::enquote(str::strencode(tokens.at(i)))));
    }

    if (tokens.at(i).str().substr(0, 1) != ".") {
        ++instruction;
        /* cout << "increased instruction count to " << instruction << ": " << tokens.at(i).str() << endl; */
    } else {
        /* cout << "not increasing instruction count: " << tokens.at(i).str() << endl; */
    }

    while (tokens.at(++i) != "\n");
    ++i;  // skip the newline
    return i;
}
