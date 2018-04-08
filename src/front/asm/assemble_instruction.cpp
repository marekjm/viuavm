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
using Token_index = vector<Token>::size_type;


static tuple<viua::internals::types::bytecode_size, enum JUMPTYPE> resolvejump(
    Token token,
    const map<string, vector<Token>::size_type>& marks,
    viua::internals::types::bytecode_size instruction_index) {
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

static string resolveregister(Token token,
                              const bool allow_bare_integers = false) {
    /*  This function is used to register numbers when a register is accessed,
     * e.g. in `integer` instruction or in `branch` in condition operand.
     *
     *  This function MUST return string as teh result is further passed to
     * assembler::operands::getint() function which *expects* string.
     */
    ostringstream out;
    string reg = token.str();
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

using ShiftOp = Program& (Program::*)(int_op, int_op, int_op);
template<const ShiftOp op>
static auto assemble_bit_shift_instruction(Program& program,
                                           const vector<Token>& tokens,
                                           const Token_index i) -> void {
    Token_index target = i + 1;
    Token_index lhs    = target + 2;
    Token_index rhs    = lhs + 2;

    int_op ret;
    if (tokens.at(target) == "void") {
        --lhs;
        --rhs;
        ret = assembler::operands::getint(resolveregister(tokens.at(target)));
    } else {
        ret = assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(target)),
            resolve_rs_type(tokens.at(target + 1)));
    }

    (program.*op)(ret,
                  assembler::operands::getint_with_rs_type(
                      resolveregister(tokens.at(lhs)),
                      resolve_rs_type(tokens.at(lhs + 1))),
                  assembler::operands::getint_with_rs_type(
                      resolveregister(tokens.at(rhs)),
                      resolve_rs_type(tokens.at(rhs + 1))));
}

using IncrementOp = Program& (Program::*)(int_op);
template<IncrementOp const op>
static auto assemble_increment_instruction(Program& program,
                                           vector<Token> const& tokens,
                                           Token_index const i) -> void {
    Token_index target = i + 1;

    (program.*op)(assembler::operands::getint_with_rs_type(
        resolveregister(tokens.at(target)),
        resolve_rs_type(tokens.at(target + 1))));
}

using ArithmeticOp = Program& (Program::*)(int_op, int_op, int_op);
template<ArithmeticOp const op>
static auto assemble_arithmetic_instruction(Program& program,
                                            vector<Token> const& tokens,
                                            Token_index const i) -> void {
    Token_index target = i + 1;
    Token_index lhs    = target + 2;
    Token_index rhs    = lhs + 2;

    (program.*op)(assembler::operands::getint_with_rs_type(
                      resolveregister(tokens.at(target)),
                      resolve_rs_type(tokens.at(target + 1))),
                  assembler::operands::getint_with_rs_type(
                      resolveregister(tokens.at(lhs)),
                      resolve_rs_type(tokens.at(lhs + 1))),
                  assembler::operands::getint_with_rs_type(
                      resolveregister(tokens.at(rhs)),
                      resolve_rs_type(tokens.at(rhs + 1))));
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

static auto assemble_op_integer(Program& program, std::vector<Token> const& tokens,
        Token_index const i) -> void {
    Token_index target = i + 1;
    Token_index source = target + 2;

    program.opinteger(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint(
                          resolveregister(tokens.at(source), true), true));
}
static auto assemble_op_vinsert(Program& program, std::vector<Token> const& tokens,
        Token_index const i) -> void {
    Token_index target   = i + 1;
    Token_index source   = target + 2;
    Token_index position = source + 2;

    auto position_op = int_op{};
    if (tokens.at(position) == "void") {
        position_op = assembler::operands::getint(
            resolveregister(tokens.at(position)));
    } else {
        position_op = assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(position)),
            resolve_rs_type(tokens.at(position + 1)));
    }

    program.opvinsert(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(source)),
                          resolve_rs_type(tokens.at(source + 1))),
                      position_op);
}
static auto assemble_op_vpop(Program& program, std::vector<Token> const& tokens,
        Token_index const i) -> void {
    Token_index target   = i + 1;
    Token_index source   = target + 2;
    Token_index position = source + 2;

    int_op target_op, source_op, position_op;

    if (tokens.at(target) == "void") {
        --source;
        --position;
        target_op =
            assembler::operands::getint(resolveregister(tokens.at(target)));
    } else {
        target_op = assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(target)),
            resolve_rs_type(tokens.at(target + 1)));
    }

    source_op = assembler::operands::getint_with_rs_type(
        resolveregister(tokens.at(source)),
        resolve_rs_type(tokens.at(source + 1)));

    if (tokens.at(position) == "void") {
        position_op = assembler::operands::getint(
            resolveregister(tokens.at(position)));
    } else {
        position_op = assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(position)),
            resolve_rs_type(tokens.at(position + 1)));
    }

    program.opvpop(target_op, source_op, position_op);
}
static auto assemble_op_bits(Program& program, std::vector<Token> const& tokens,
        Token_index const i) -> void {
    Token_index target = i + 1;
    Token_index lhs    = target + 2;

    auto src = tokens.at(lhs).str();
    if (src.at(0) == '0'
        and (src.at(1) == 'b' or src.at(1) == 'o' or src.at(1) == 'x')) {
        program.opbits(
            assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(target)),
                resolve_rs_type(tokens.at(target + 1))),
            assembler::operands::convert_token_to_bitstring_operand(
                tokens.at(lhs)));
    } else {
        program.opbits(assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(target)),
                           resolve_rs_type(tokens.at(target + 1))),
                       assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(lhs)),
                           resolve_rs_type(tokens.at(lhs + 1))));
    }
}
static auto assemble_op_bitset(Program& program, std::vector<Token> const& tokens,
        Token_index const i) -> void {
    Token_index target = i + 1;
    Token_index lhs    = target + 2;
    Token_index rhs    = lhs + 2;

    if (tokens.at(rhs) == "true" or tokens.at(rhs) == "false") {
        program.opbitset(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(lhs)),
                             resolve_rs_type(tokens.at(lhs + 1))),
                         (tokens.at(rhs) == "true"));
    } else {
        program.opbitset(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(lhs)),
                             resolve_rs_type(tokens.at(lhs + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(rhs)),
                             resolve_rs_type(tokens.at(rhs + 1))));
    }
}
static auto assemble_op_call(Program& program, std::vector<Token> const& tokens,
        Token_index const i) -> void {
    /** Full form of call instruction has two operands: function name and
     *  return value register index.
     *  If call is given only one operand it means it is the function name
     * and returned value is discarded. To explicitly state that return
     * value should be discarderd put `void` as return register index.
     */
    /** Why is the function supplied as a *string* and
     *  not direct instruction pointer?
     *  That would be faster - c'mon couldn't assembler just calculate
     * offsets and insert them?
     *
     *  Nope.
     *
     *  Yes, it *would* be faster if calls were just precalculated jumps.
     *  However, by them being strings we get plenty of flexibility,
     * good-quality stack traces, and a place to put plenty of debugging
     * info. All that at a cost of just one map lookup; the overhead is
     * minimal and gains are big. What's not to love?
     *
     *  Of course, you, my dear reader, are free to take this code (it's GPL
     * after all!) and modify it to suit your particular needs - in that
     * case that would be calculating call jumps at compile time and
     * exchanging CALL instructions with JUMP instructions.
     *
     *  Good luck with debugging your code, then.
     */
    Token_index target = i + 1;
    Token_index fn     = target + 2;

    int_op ret;
    if (tokens.at(target) == "void") {
        --fn;
        ret =
            assembler::operands::getint(resolveregister(tokens.at(target)));
    } else {
        ret = assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(target)),
            resolve_rs_type(tokens.at(target + 1)));
    }

    if (tokens.at(fn).str().at(0) == '*'
        or tokens.at(fn).str().at(0) == '%') {
        program.opcall(ret,
                       assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(fn)),
                           resolve_rs_type(tokens.at(fn + 1))));
    } else {
        program.opcall(ret, tokens.at(fn));
    }
}
static auto assemble_op_if(Program& program, std::vector<Token> const& tokens,
        Token_index const i,
        viua::internals::types::bytecode_size const& instruction,
        std::map<std::string, std::remove_reference<decltype(tokens)>::type::size_type> const& marks) -> void {
        /*  If branch is given three operands, it means its full, three-operands
         * form is being used. Otherwise, it is short, two-operands form
         * instruction and assembler should fill third operand accordingly.
         *
         *  In case of short-form `branch` instruction:
         *
         *      * first operand is index of the register to check,
         *      * second operand is the address to which to jump if register is
         * true,
         *      * third operand is assumed to be the *next instruction*, i.e.
         * instruction after the branch instruction,
         *
         *  In full (with three operands) form of `branch` instruction:
         *
         *      * third operands is the address to which to jump if register is
         * false,
         */
        Token condition = tokens.at(i + 1);
        Token if_true   = tokens.at(i + 3);
        Token if_false  = tokens.at(i + 4);

        viua::internals::types::bytecode_size addrt_target, addrf_target;
        enum JUMPTYPE addrt_jump_type, addrf_jump_type;
        tie(addrt_target, addrt_jump_type) =
            resolvejump(tokens.at(i + 3), marks, instruction);
        if (if_false != "\n") {
            tie(addrf_target, addrf_jump_type) =
                resolvejump(tokens.at(i + 4), marks, instruction);
        } else {
            addrf_jump_type = JMP_RELATIVE;
            addrf_target    = instruction + 1;
        }

        program.opif(
            assembler::operands::getint_with_rs_type(
                resolveregister(condition), resolve_rs_type(tokens.at(i + 2))),
            addrt_target,
            addrt_jump_type,
            addrf_target,
            addrf_jump_type);
}
static auto assemble_op_jump(Program& program, std::vector<Token> const& tokens,
        Token_index const i,
        viua::internals::types::bytecode_size const& instruction,
        std::map<std::string, std::remove_reference<decltype(tokens)>::type::size_type> const& marks) -> void {
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
     *  If it is a marker jump, assembler will look the marker up in a map
     * and if it is not found throw an exception about unrecognised marker
     * being used.
     */
    viua::internals::types::bytecode_size jump_target;
    enum JUMPTYPE jump_type;
    tie(jump_target, jump_type) =
        resolvejump(tokens.at(i + 1), marks, instruction);

    program.opjump(jump_target, jump_type);
}
static auto assemble_op_structremove(Program& program, std::vector<Token> const& tokens,
        Token_index const i) -> void {
    Token_index target = i + 1;
    Token_index source = target + 2;
    Token_index key    = source + 2;

    if (tokens.at(target) == "void") {
        --source;
        --key;
        program.opstructremove(
            assembler::operands::getint(resolveregister(tokens.at(target))),
            assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(source)),
                resolve_rs_type(tokens.at(source + 1))),
            assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(key)),
                resolve_rs_type(tokens.at(key + 1))));
    } else {
        program.opstructremove(assembler::operands::getint_with_rs_type(
                                   resolveregister(tokens.at(target)),
                                   resolve_rs_type(tokens.at(target + 1))),
                               assembler::operands::getint_with_rs_type(
                                   resolveregister(tokens.at(source)),
                                   resolve_rs_type(tokens.at(source + 1))),
                               assembler::operands::getint_with_rs_type(
                                   resolveregister(tokens.at(key)),
                                   resolve_rs_type(tokens.at(key + 1))));
    }
}
static auto assemble_op_msg(Program& program, std::vector<Token> const& tokens,
        Token_index const i) -> void {
        Token_index target = i + 1;
        Token_index fn     = target + 2;

        int_op ret;
        if (tokens.at(target) == "void") {
            --fn;
            ret =
                assembler::operands::getint(resolveregister(tokens.at(target)));
        } else {
            ret = assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(target)),
                resolve_rs_type(tokens.at(target + 1)));
        }

        if (tokens.at(fn).str().at(0) == '*'
            or tokens.at(fn).str().at(0) == '%') {
            program.opmsg(ret,
                          assembler::operands::getint_with_rs_type(
                              resolveregister(tokens.at(fn)),
                              resolve_rs_type(tokens.at(fn + 1))));
        } else {
            program.opmsg(ret, tokens.at(fn));
        }
}

viua::internals::types::bytecode_size assemble_instruction(
    Program& program,
    viua::internals::types::bytecode_size& instruction,
    viua::internals::types::bytecode_size i,
    const vector<Token>& tokens,
    map<string, std::remove_reference<decltype(tokens)>::type::size_type>&
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
        Token_index target = i + 1;

        program.opizero(assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(target)),
            resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "integer") {
        assemble_op_integer(program, tokens, i);
    } else if (tokens.at(i) == "iinc") {
        Token_index target = i + 1;

        program.opiinc(assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(target)),
            resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "idec") {
        Token_index target = i + 1;

        program.opidec(assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(target)),
            resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "float") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opfloat(assembler::operands::getint_with_rs_type(
                            resolveregister(tokens.at(target)),
                            resolve_rs_type(tokens.at(target + 1))),
                        stod(tokens.at(source).str()));
    } else if (tokens.at(i) == "itof") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opitof(assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(target)),
                           resolve_rs_type(tokens.at(target + 1))),
                       assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(source)),
                           resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "ftoi") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opftoi(assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(target)),
                           resolve_rs_type(tokens.at(target + 1))),
                       assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(source)),
                           resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "stoi") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opstoi(assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(target)),
                           resolve_rs_type(tokens.at(target + 1))),
                       assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(source)),
                           resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "stof") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opstof(assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(target)),
                           resolve_rs_type(tokens.at(target + 1))),
                       assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(source)),
                           resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "add") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opadd(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(lhs)),
                          resolve_rs_type(tokens.at(lhs + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(rhs)),
                          resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "sub") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opsub(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(lhs)),
                          resolve_rs_type(tokens.at(lhs + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(rhs)),
                          resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "mul") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opmul(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(lhs)),
                          resolve_rs_type(tokens.at(lhs + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(rhs)),
                          resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "div") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opdiv(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(lhs)),
                          resolve_rs_type(tokens.at(lhs + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(rhs)),
                          resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "lt") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.oplt(assembler::operands::getint_with_rs_type(
                         resolveregister(tokens.at(target)),
                         resolve_rs_type(tokens.at(target + 1))),
                     assembler::operands::getint_with_rs_type(
                         resolveregister(tokens.at(lhs)),
                         resolve_rs_type(tokens.at(lhs + 1))),
                     assembler::operands::getint_with_rs_type(
                         resolveregister(tokens.at(rhs)),
                         resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "lte") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.oplte(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(lhs)),
                          resolve_rs_type(tokens.at(lhs + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(rhs)),
                          resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "gt") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opgt(assembler::operands::getint_with_rs_type(
                         resolveregister(tokens.at(target)),
                         resolve_rs_type(tokens.at(target + 1))),
                     assembler::operands::getint_with_rs_type(
                         resolveregister(tokens.at(lhs)),
                         resolve_rs_type(tokens.at(lhs + 1))),
                     assembler::operands::getint_with_rs_type(
                         resolveregister(tokens.at(rhs)),
                         resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "gte") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opgte(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(lhs)),
                          resolve_rs_type(tokens.at(lhs + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(rhs)),
                          resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "eq") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opeq(assembler::operands::getint_with_rs_type(
                         resolveregister(tokens.at(target)),
                         resolve_rs_type(tokens.at(target + 1))),
                     assembler::operands::getint_with_rs_type(
                         resolveregister(tokens.at(lhs)),
                         resolve_rs_type(tokens.at(lhs + 1))),
                     assembler::operands::getint_with_rs_type(
                         resolveregister(tokens.at(rhs)),
                         resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "string") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opstring(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         tokens.at(source));
    } else if (tokens.at(i) == "text") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        auto target_operand = assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(target)),
            resolve_rs_type(tokens.at(target + 1)));
        if (tokens.at(source).str().at(0) == '*'
            or tokens.at(source).str().at(0) == '%') {
            program.optext(target_operand,
                           assembler::operands::getint_with_rs_type(
                               resolveregister(tokens.at(source)),
                               resolve_rs_type(tokens.at(source + 1))));
        } else {
            program.optext(target_operand, tokens.at(source));
        }
    } else if (tokens.at(i) == "texteq") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.optexteq(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(lhs)),
                             resolve_rs_type(tokens.at(lhs + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(rhs)),
                             resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "textat") {
        Token_index target = i + 1;
        Token_index source = target + 2;
        Token_index index  = source + 2;

        program.optextat(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(source)),
                             resolve_rs_type(tokens.at(source + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(index)),
                             resolve_rs_type(tokens.at(index + 1))));
    } else if (tokens.at(i) == "textsub") {
        Token_index target      = i + 1;
        Token_index source      = target + 2;
        Token_index begin_index = source + 2;
        Token_index end_index   = begin_index + 2;

        program.optextsub(assembler::operands::getint_with_rs_type(
                              resolveregister(tokens.at(target)),
                              resolve_rs_type(tokens.at(target + 1))),
                          assembler::operands::getint_with_rs_type(
                              resolveregister(tokens.at(source)),
                              resolve_rs_type(tokens.at(source + 1))),
                          assembler::operands::getint_with_rs_type(
                              resolveregister(tokens.at(begin_index)),
                              resolve_rs_type(tokens.at(begin_index + 1))),
                          assembler::operands::getint_with_rs_type(
                              resolveregister(tokens.at(end_index)),
                              resolve_rs_type(tokens.at(end_index + 1))));
    } else if (tokens.at(i) == "textlength") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.optextlength(assembler::operands::getint_with_rs_type(
                                 resolveregister(tokens.at(target)),
                                 resolve_rs_type(tokens.at(target + 1))),
                             assembler::operands::getint_with_rs_type(
                                 resolveregister(tokens.at(source)),
                                 resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "textcommonprefix") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.optextcommonprefix(assembler::operands::getint_with_rs_type(
                                       resolveregister(tokens.at(target)),
                                       resolve_rs_type(tokens.at(target + 1))),
                                   assembler::operands::getint_with_rs_type(
                                       resolveregister(tokens.at(lhs)),
                                       resolve_rs_type(tokens.at(lhs + 1))),
                                   assembler::operands::getint_with_rs_type(
                                       resolveregister(tokens.at(rhs)),
                                       resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "textcommonsuffix") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.optextcommonsuffix(assembler::operands::getint_with_rs_type(
                                       resolveregister(tokens.at(target)),
                                       resolve_rs_type(tokens.at(target + 1))),
                                   assembler::operands::getint_with_rs_type(
                                       resolveregister(tokens.at(lhs)),
                                       resolve_rs_type(tokens.at(lhs + 1))),
                                   assembler::operands::getint_with_rs_type(
                                       resolveregister(tokens.at(rhs)),
                                       resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "textconcat") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.optextconcat(assembler::operands::getint_with_rs_type(
                                 resolveregister(tokens.at(target)),
                                 resolve_rs_type(tokens.at(target + 1))),
                             assembler::operands::getint_with_rs_type(
                                 resolveregister(tokens.at(lhs)),
                                 resolve_rs_type(tokens.at(lhs + 1))),
                             assembler::operands::getint_with_rs_type(
                                 resolveregister(tokens.at(rhs)),
                                 resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "vector") {
        Token_index target           = i + 1;
        Token_index pack_range_start = target + 2;
        Token_index pack_range_count = pack_range_start + 2;

        program.opvector(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(pack_range_start)),
                             resolve_rs_type(tokens.at(pack_range_start + 1))),
                         assembler::operands::getint(
                             resolveregister(tokens.at(pack_range_count))));
    } else if (tokens.at(i) == "vinsert") {
        assemble_op_vinsert(program, tokens, i);
    } else if (tokens.at(i) == "vpush") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opvpush(assembler::operands::getint_with_rs_type(
                            resolveregister(tokens.at(target)),
                            resolve_rs_type(tokens.at(target + 1))),
                        assembler::operands::getint_with_rs_type(
                            resolveregister(tokens.at(source)),
                            resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "vpop") {
        assemble_op_vpop(program, tokens, i);
    } else if (tokens.at(i) == "vat") {
        Token_index target   = i + 1;
        Token_index source   = target + 2;
        Token_index position = source + 2;

        program.opvat(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(source)),
                          resolve_rs_type(tokens.at(source + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(position)),
                          resolve_rs_type(tokens.at(position + 1))));
    } else if (tokens.at(i) == "vlen") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opvlen(assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(target)),
                           resolve_rs_type(tokens.at(target + 1))),
                       assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(source)),
                           resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "not") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opnot(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(source)),
                          resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "and") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opand(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(lhs)),
                          resolve_rs_type(tokens.at(lhs + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(rhs)),
                          resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "or") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opor(assembler::operands::getint_with_rs_type(
                         resolveregister(tokens.at(target)),
                         resolve_rs_type(tokens.at(target + 1))),
                     assembler::operands::getint_with_rs_type(
                         resolveregister(tokens.at(lhs)),
                         resolve_rs_type(tokens.at(lhs + 1))),
                     assembler::operands::getint_with_rs_type(
                         resolveregister(tokens.at(rhs)),
                         resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "bits") {
        assemble_op_bits(program, tokens, i);
    } else if (tokens.at(i) == "bitand") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opbitand(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(lhs)),
                             resolve_rs_type(tokens.at(lhs + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(rhs)),
                             resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "bitor") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opbitor(assembler::operands::getint_with_rs_type(
                            resolveregister(tokens.at(target)),
                            resolve_rs_type(tokens.at(target + 1))),
                        assembler::operands::getint_with_rs_type(
                            resolveregister(tokens.at(lhs)),
                            resolve_rs_type(tokens.at(lhs + 1))),
                        assembler::operands::getint_with_rs_type(
                            resolveregister(tokens.at(rhs)),
                            resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "bitnot") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;

        program.opbitnot(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(lhs)),
                             resolve_rs_type(tokens.at(lhs + 1))));
    } else if (tokens.at(i) == "bitxor") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opbitxor(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(lhs)),
                             resolve_rs_type(tokens.at(lhs + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(rhs)),
                             resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "bitat") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opbitat(assembler::operands::getint_with_rs_type(
                            resolveregister(tokens.at(target)),
                            resolve_rs_type(tokens.at(target + 1))),
                        assembler::operands::getint_with_rs_type(
                            resolveregister(tokens.at(lhs)),
                            resolve_rs_type(tokens.at(lhs + 1))),
                        assembler::operands::getint_with_rs_type(
                            resolveregister(tokens.at(rhs)),
                            resolve_rs_type(tokens.at(rhs + 1))));
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
        Token_index target = i + 1;
        Token_index lhs    = target + 2;

        program.oprol(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(lhs)),
                          resolve_rs_type(tokens.at(lhs + 1))));
    } else if (tokens.at(i) == "ror") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;

        program.opror(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(lhs)),
                          resolve_rs_type(tokens.at(lhs + 1))));
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
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opmove(assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(target)),
                           resolve_rs_type(tokens.at(target + 1))),
                       assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(source)),
                           resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "copy") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opcopy(assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(target)),
                           resolve_rs_type(tokens.at(target + 1))),
                       assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(source)),
                           resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "ptr") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opptr(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(source)),
                          resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "swap") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opswap(assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(target)),
                           resolve_rs_type(tokens.at(target + 1))),
                       assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(source)),
                           resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "delete") {
        Token_index target = i + 1;

        program.opdelete(assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(target)),
            resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "isnull") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opisnull(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(source)),
                             resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "print") {
        Token_index source = i + 1;

        program.opprint(assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(source)),
            resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "echo") {
        Token_index source = i + 1;

        program.opecho(assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(source)),
            resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "capture") {
        Token_index target       = i + 1;
        Token_index inside_index = target + 2;
        Token_index source       = inside_index + 1;

        program.opcapture(assembler::operands::getint_with_rs_type(
                              resolveregister(tokens.at(target)),
                              resolve_rs_type(tokens.at(target + 1))),
                          assembler::operands::getint(
                              resolveregister(tokens.at(inside_index))),
                          assembler::operands::getint_with_rs_type(
                              resolveregister(tokens.at(source)),
                              resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "capturecopy") {
        Token_index target       = i + 1;
        Token_index inside_index = target + 2;
        Token_index source       = inside_index + 1;

        program.opcapturecopy(assembler::operands::getint_with_rs_type(
                                  resolveregister(tokens.at(target)),
                                  resolve_rs_type(tokens.at(target + 1))),
                              assembler::operands::getint(
                                  resolveregister(tokens.at(inside_index))),
                              assembler::operands::getint_with_rs_type(
                                  resolveregister(tokens.at(source)),
                                  resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "capturemove") {
        Token_index target       = i + 1;
        Token_index inside_index = target + 2;
        Token_index source       = inside_index + 1;

        program.opcapturemove(assembler::operands::getint_with_rs_type(
                                  resolveregister(tokens.at(target)),
                                  resolve_rs_type(tokens.at(target + 1))),
                              assembler::operands::getint(
                                  resolveregister(tokens.at(inside_index))),
                              assembler::operands::getint_with_rs_type(
                                  resolveregister(tokens.at(source)),
                                  resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "closure") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opclosure(assembler::operands::getint_with_rs_type(
                              resolveregister(tokens.at(target)),
                              resolve_rs_type(tokens.at(target + 1))),
                          tokens.at(source));
    } else if (tokens.at(i) == "function") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opfunction(assembler::operands::getint_with_rs_type(
                               resolveregister(tokens.at(target)),
                               resolve_rs_type(tokens.at(target + 1))),
                           tokens.at(source));
    } else if (tokens.at(i) == "frame") {
        Token_index target = i + 1;
        Token_index source = target + 1;

        program.opframe(
            assembler::operands::getint(resolveregister(tokens.at(target))),
            assembler::operands::getint(resolveregister(tokens.at(source))));
    } else if (tokens.at(i) == "param") {
        Token_index target = i + 1;
        Token_index source = target + 1;

        program.opparam(
            assembler::operands::getint(resolveregister(tokens.at(target))),
            assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(source)),
                resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "pamv") {
        Token_index target = i + 1;
        Token_index source = target + 1;

        program.oppamv(
            assembler::operands::getint(resolveregister(tokens.at(target))),
            assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(source)),
                resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "arg") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        if (tokens.at(target) == "void") {
            --source;
            program.oparg(
                assembler::operands::getint(resolveregister(tokens.at(target))),
                assembler::operands::getint(
                    resolveregister(tokens.at(source))));
        } else {
            program.oparg(assembler::operands::getint_with_rs_type(
                              resolveregister(tokens.at(target)),
                              resolve_rs_type(tokens.at(target + 1))),
                          assembler::operands::getint(
                              resolveregister(tokens.at(source))));
        }
    } else if (tokens.at(i) == "argc") {
        Token_index target = i + 1;

        program.opargc(assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(target)),
            resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "call") {
        assemble_op_call(program, tokens, i);
    } else if (tokens.at(i) == "tailcall") {
        if (tokens.at(i + 1).str().at(0) == '*'
            or tokens.at(i + 1).str().at(0) == '%') {
            program.optailcall(assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(i + 1)),
                resolve_rs_type(tokens.at(i + 2))));
        } else {
            program.optailcall(tokens.at(i + 1));
        }
    } else if (tokens.at(i) == "defer") {
        if (tokens.at(i + 1).str().at(0) == '*'
            or tokens.at(i + 1).str().at(0) == '%') {
            program.opdefer(assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(i + 1)),
                resolve_rs_type(tokens.at(i + 2))));
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
                assembler::operands::getint(resolveregister(tokens.at(target)));
        } else {
            ret = assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(target)),
                resolve_rs_type(tokens.at(target + 1)));
        }

        program.opprocess(ret, tokens.at(fn));
    } else if (tokens.at(i) == "self") {
        Token_index target = i + 1;

        program.opself(assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(target)),
            resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "join") {
        Token_index target        = i + 1;
        Token_index process       = target + 2;
        Token_index timeout_index = process + 2;

        int_op target_operand;
        if (tokens.at(target) == "void") {
            --process;
            --timeout_index;
            target_operand =
                assembler::operands::getint(resolveregister(tokens.at(target)));
        } else {
            target_operand = assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(target)),
                resolve_rs_type(tokens.at(target + 1)));
        }

        timeout_op timeout =
            convert_token_to_timeout_operand(tokens.at(timeout_index));
        program.opjoin(target_operand,
                       assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(process)),
                           resolve_rs_type(tokens.at(process + 1))),
                       timeout);
    } else if (tokens.at(i) == "send") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opsend(assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(target)),
                           resolve_rs_type(tokens.at(target + 1))),
                       assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(source)),
                           resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "receive") {
        Token_index target        = i + 1;
        Token_index timeout_index = target + 2;

        int_op target_operand;
        if (tokens.at(target) == "void") {
            --timeout_index;
            target_operand =
                assembler::operands::getint(resolveregister(tokens.at(target)));
        } else {
            target_operand = assembler::operands::getint_with_rs_type(
                resolveregister(tokens.at(target)),
                resolve_rs_type(tokens.at(target + 1)));
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
            resolveregister(tokens.at(target)),
            resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "enter") {
        program.openter(tokens.at(i + 1));
    } else if (tokens.at(i) == "throw") {
        Token_index source = i + 1;

        program.opthrow(assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(source)),
            resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "leave") {
        program.opleave();
    } else if (tokens.at(i) == "import") {
        program.opimport(tokens.at(i + 1));
    } else if (tokens.at(i) == "class") {
        Token_index target     = i + 1;
        Token_index class_name = target + 2;

        program.opclass(assembler::operands::getint_with_rs_type(
                            resolveregister(tokens.at(target)),
                            resolve_rs_type(tokens.at(target + 1))),
                        tokens.at(class_name));
    } else if (tokens.at(i) == "derive") {
        Token_index target     = i + 1;
        Token_index class_name = target + 2;

        program.opderive(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         tokens.at(class_name));
    } else if (tokens.at(i) == "attach") {
        Token_index target        = i + 1;
        Token_index fn_name       = target + 2;
        Token_index attached_name = fn_name + 1;

        program.opattach(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         tokens.at(fn_name),
                         tokens.at(attached_name));
    } else if (tokens.at(i) == "register") {
        Token_index target = i + 1;

        program.opregister(assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(target)),
            resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "atom") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opatom(assembler::operands::getint_with_rs_type(
                           resolveregister(tokens.at(target)),
                           resolve_rs_type(tokens.at(target + 1))),
                       tokens.at(source));
    } else if (tokens.at(i) == "atomeq") {
        Token_index target = i + 1;
        Token_index lhs    = target + 2;
        Token_index rhs    = lhs + 2;

        program.opatomeq(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(lhs)),
                             resolve_rs_type(tokens.at(lhs + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(rhs)),
                             resolve_rs_type(tokens.at(rhs + 1))));
    } else if (tokens.at(i) == "struct") {
        Token_index target = i + 1;

        program.opstruct(assembler::operands::getint_with_rs_type(
            resolveregister(tokens.at(target)),
            resolve_rs_type(tokens.at(target + 1))));
    } else if (tokens.at(i) == "structinsert") {
        Token_index target = i + 1;
        Token_index key    = target + 2;
        Token_index source = key + 2;

        program.opstructinsert(assembler::operands::getint_with_rs_type(
                                   resolveregister(tokens.at(target)),
                                   resolve_rs_type(tokens.at(target + 1))),
                               assembler::operands::getint_with_rs_type(
                                   resolveregister(tokens.at(key)),
                                   resolve_rs_type(tokens.at(key + 1))),
                               assembler::operands::getint_with_rs_type(
                                   resolveregister(tokens.at(source)),
                                   resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "structremove") {
        assemble_op_structremove(program, tokens, i);
    } else if (tokens.at(i) == "structkeys") {
        Token_index target = i + 1;
        Token_index source = target + 2;

        program.opstructkeys(assembler::operands::getint_with_rs_type(
                                 resolveregister(tokens.at(target)),
                                 resolve_rs_type(tokens.at(target + 1))),
                             assembler::operands::getint_with_rs_type(
                                 resolveregister(tokens.at(source)),
                                 resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "new") {
        Token_index target     = i + 1;
        Token_index class_name = target + 2;

        program.opnew(assembler::operands::getint_with_rs_type(
                          resolveregister(tokens.at(target)),
                          resolve_rs_type(tokens.at(target + 1))),
                      tokens.at(class_name));
    } else if (tokens.at(i) == "msg") {
        assemble_op_msg(program, tokens, i);
    } else if (tokens.at(i) == "insert") {
        Token_index target = i + 1;
        Token_index key    = target + 2;
        Token_index source = key + 2;

        program.opinsert(assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(target)),
                             resolve_rs_type(tokens.at(target + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(key)),
                             resolve_rs_type(tokens.at(key + 1))),
                         assembler::operands::getint_with_rs_type(
                             resolveregister(tokens.at(source)),
                             resolve_rs_type(tokens.at(source + 1))));
    } else if (tokens.at(i) == "remove") {
        Token_index target = i + 1;
        Token_index source = target + 2;
        Token_index key    = source + 2;

        if (tokens.at(target) == "void") {
            --source;
            --key;
            program.opremove(
                assembler::operands::getint(resolveregister(tokens.at(target))),
                assembler::operands::getint_with_rs_type(
                    resolveregister(tokens.at(source)),
                    resolve_rs_type(tokens.at(source + 1))),
                assembler::operands::getint_with_rs_type(
                    resolveregister(tokens.at(key)),
                    resolve_rs_type(tokens.at(key + 1))));
        } else {
            program.opremove(assembler::operands::getint_with_rs_type(
                                 resolveregister(tokens.at(target)),
                                 resolve_rs_type(tokens.at(target + 1))),
                             assembler::operands::getint_with_rs_type(
                                 resolveregister(tokens.at(source)),
                                 resolve_rs_type(tokens.at(source + 1))),
                             assembler::operands::getint_with_rs_type(
                                 resolveregister(tokens.at(key)),
                                 resolve_rs_type(tokens.at(key + 1))));
        }
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
        /* cout << "increased instruction count to " << instruction << ": " <<
         * tokens.at(i).str() << endl; */
    } else {
        /* cout << "not increasing instruction count: " << tokens.at(i).str() <<
         * endl; */
    }

    while (tokens.at(++i) != "\n")
        ;
    ++i;  // skip the newline
    return i;
}
