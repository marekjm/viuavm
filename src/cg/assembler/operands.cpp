/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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
#include <string>
#include <tuple>
#include <vector>

#include <viua/cg/assembler/assembler.h>
#include <viua/cg/lex.h>
#include <viua/program.h>
#include <viua/support/string.h>


static auto resolveregister(viua::cg::lex::Token const token,
                            bool const allow_bare_integers = false)
    -> std::string
{
    /*  This function is used to register numbers when a register is accessed,
     * e.g. in `integer` instruction or in `branch` in condition operand.
     *
     *  This function MUST return std::string as teh result is further passed to
     * assembler::operands::getint() function which *expects* std::string.
     */
    auto out       = std::ostringstream{};
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
        throw viua::cg::lex::Invalid_syntax(
            token, ("illegal operand: " + token.str()));
    }
    return out.str();
}


auto assembler::operands::getint(std::string const& s,
                                 bool const allow_bare_integers) -> int_op
{
    if (s.size() == 0) {
        throw "empty std::string cannot be used as operand";
    }

    if (s == "void") {
        return int_op(Integer_operand_type::VOID);
    } else if (s.at(0) == '@') {
        return int_op(Integer_operand_type::REGISTER_REFERENCE,
                      stoi(s.substr(1)));
    } else if (s.at(0) == '*') {
        return int_op(Integer_operand_type::POINTER_DEREFERENCE,
                      stoi(s.substr(1)));
    } else if (s.at(0) == '%') {
        return int_op(stoi(s.substr(1)));
    } else if (allow_bare_integers and str::isnum(s)) {
        return int_op(stoi(s));
    } else {
        throw("cannot convert to int operand: " + s);
    }
}

auto assembler::operands::getint_with_rs_type(
    std::string const& s,
    viua::bytecode::codec::Register_set const rs_type,
    bool const allow_bare_integers) -> int_op
{
    if (s.size() == 0) {
        throw "empty std::string cannot be used as operand";
    }

    if (s == "void") {
        return int_op(Integer_operand_type::VOID);
    } else if (s.at(0) == '@') {
        return int_op(Integer_operand_type::REGISTER_REFERENCE,
                      rs_type,
                      stoi(s.substr(1)));
    } else if (s.at(0) == '*') {
        return int_op(Integer_operand_type::POINTER_DEREFERENCE,
                      rs_type,
                      stoi(s.substr(1)));
    } else if (s.at(0) == '%') {
        return int_op(Integer_operand_type::INDEX, rs_type, stoi(s.substr(1)));
    } else if (allow_bare_integers and str::isnum(s)) {
        return int_op(stoi(s));
    } else {
        throw("cannot convert to int operand: " + s);
    }
}

auto assembler::operands::getint(
    std::vector<viua::cg::lex::Token> const& tokens,
    decltype(tokens.size()) const i) -> int_op
{
    auto const s = resolveregister(tokens.at(i));

    if (s.size() == 0) {
        throw "empty std::string cannot be used as operand";
    }

    if (s == "void") {
        return int_op(Integer_operand_type::VOID);
    }

    auto iop = int_op{};
    if (s.at(0) == '@') {
        iop =
            int_op(Integer_operand_type::REGISTER_REFERENCE, stoi(s.substr(1)));
    } else if (s.at(0) == '*') {
        iop = int_op(Integer_operand_type::POINTER_DEREFERENCE,
                     stoi(s.substr(1)));
    } else if (s.at(0) == '%') {
        iop = int_op(stoi(s.substr(1)));
    } else {
        throw viua::cg::lex::Invalid_syntax(tokens.at(i),
                                            "cannot convert to register index");
    }

    // FIXME set iop.rs_type according to rs specifier for given operand

    return iop;
}

auto assembler::operands::getbyte(std::string const& s) -> byte_op
{
    auto const ref = (s[0] == '@');
    return std::tuple<bool, char>(
        ref, static_cast<char>(stoi(ref ? str::sub(s, 1) : s)));
}

auto assembler::operands::getfloat(std::string const& s) -> float_op
{
    auto const ref = (s[0] == '@');
    return std::tuple<bool, float>(ref, stof(ref ? str::sub(s, 1) : s));
}

auto assembler::operands::get2(std::string const s)
    -> std::tuple<std::string, std::string>
{
    /** Returns std::tuple of two strings - two operands chunked from the `s`
     * std::string.
     */
    auto const op_a = str::chunk(s);
    auto const op_b = str::chunk(str::sub(s, op_a.size()));
    return std::tuple<std::string, std::string>(op_a, op_b);
}

auto assembler::operands::get3(std::string const s, bool const fill_third)
    -> std::tuple<std::string, std::string, std::string>
{
    auto const op_a      = str::chunk(s);
    auto const s_after_a = str::lstrip(str::sub(s, op_a.size()));

    auto const op_b      = str::chunk(s_after_a);
    auto const s_after_b = str::lstrip(str::sub(s_after_a, op_b.size()));

    /* If s is empty and fill_third is true, use first operand as a filler.
     * In any other case, use the chunk of s.
     * The chunk of empty std::string will give us empty std::string and
     * it is a valid (and sometimes wanted) value to return.
     */
    auto const op_c =
        (s.size() == 0 and fill_third ? op_a : str::chunk(s_after_b));

    return std::tuple<std::string, std::string, std::string>(op_a, op_b, op_c);
}

auto assembler::operands::convert_token_to_bitstring_operand(
    viua::cg::lex::Token const token) -> std::vector<uint8_t>
{
    auto const s            = token.str();
    auto normalised_version = std::string{};
    if (s.at(1) == 'b') {
        normalised_version = normalise_binary_literal(s.substr(2));
    } else if (s.at(1) == 'o') {
        normalised_version =
            normalise_binary_literal(octal_to_binary_literal(s));
    } else if (s.at(1) == 'x') {
        normalised_version =
            normalise_binary_literal(hexadecimal_to_binary_literal(s));
    } else {
        throw viua::cg::lex::Invalid_syntax(token);
    }

    reverse(normalised_version.begin(), normalised_version.end());
    auto const workable_version = normalised_version;

    auto converted = std::vector<uint8_t>{};
    auto part      = uint8_t{0};
    for (auto i = decltype(workable_version)::size_type{0};
         i < workable_version.size();
         ++i) {
        auto one = uint8_t{1};
        if (workable_version.at(i) == '1') {
            one  = static_cast<uint8_t>(one << (i % 8));
            part = (part | one);
        }
        if ((i + 1) % 8 == 0) {
            converted.push_back(part);
            part = 0;
        }
    }

    return converted;
}
