/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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
#include <tuple>
#include <vector>
#include <viua/support/string.h>
#include <viua/cg/lex.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/program.h>
using namespace std;


static string resolveregister(viua::cg::lex::Token token) {
    /*  This function is used to register numbers when a register is accessed, e.g.
     *  in `istore` instruction or in `branch` in condition operand.
     *
     *  This function MUST return string as teh result is further passed to assembler::operands::getint() function which *expects* string.
     */
    ostringstream out;
    string reg = token.str();
    if (str::isnum(reg)) {
        /*  Basic case - the register is accessed as real index, everything is nice and simple.
         */
        out.str(reg);
    } else if (reg[0] == '@' and str::isnum(str::sub(reg, 1))) {
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
    } else if (reg == "void") {
        out << reg;
    } else {
        throw viua::cg::lex::InvalidSyntax(token, "not enough operands");
    }
    return out.str();
}

int_op assembler::operands::getint(const string& s) {
    if (s.size() == 0) {
        throw "empty string cannot be used as operand";
    }

    if (s == "void") {
        return int_op(IntegerOperandType::VOID);
    }
    if (s.at(0) == '@') {
        return int_op(IntegerOperandType::REGISTER_REFERENCE, stoi(s.substr(1)));
    }
    if (s.at(0) == '*') {
        return int_op(IntegerOperandType::POINTER_DEREFERENCE, stoi(s.substr(1)));
    }

    return int_op(stoi(s));
}

int_op assembler::operands::getint(const vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) {
    string s = resolveregister(tokens.at(i));

    if (s.size() == 0) {
        throw "empty string cannot be used as operand";
    }

    if (s == "void") {
        return int_op(IntegerOperandType::VOID);
    }

    int_op iop;
    if (s.at(0) == '@') {
        iop = int_op(IntegerOperandType::REGISTER_REFERENCE, stoi(s.substr(1)));
    } else if (s.at(0) == '*') {
        iop = int_op(IntegerOperandType::POINTER_DEREFERENCE, stoi(s.substr(1)));
    } else {
        iop = int_op(stoi(s));
    }

    auto previous_token = tokens.at(i-1);
    if (previous_token == "static") {
        iop.rs_type = viua::internals::RegisterSets::STATIC;
    } else if (previous_token == "local") {
        iop.rs_type = viua::internals::RegisterSets::LOCAL;
    } else if (previous_token == "global") {
        iop.rs_type = viua::internals::RegisterSets::GLOBAL;
    } else if (previous_token == "\n") {
        // do nothing
    } else if (str::isnum(previous_token) or str::isid(previous_token)) {
        // do nothing
    } else {
        throw viua::cg::lex::InvalidSyntax(previous_token, "unexpected token");
    }

    return iop;
}

byte_op assembler::operands::getbyte(const string& s) {
    bool ref = s[0] == '@';
    return tuple<bool, char>(ref, static_cast<char>(stoi(ref ? str::sub(s, 1) : s)));
}

float_op assembler::operands::getfloat(const string& s) {
    bool ref = s[0] == '@';
    return tuple<bool, float>(ref, stof(ref ? str::sub(s, 1) : s));
}

tuple<string, string> assembler::operands::get2(string s) {
    /** Returns tuple of two strings - two operands chunked from the `s` string.
     */
    string op_a, op_b;
    op_a = str::chunk(s);
    s = str::sub(s, op_a.size());
    op_b = str::chunk(s);
    return tuple<string, string>(op_a, op_b);
}

tuple<string, string, string> assembler::operands::get3(string s, bool fill_third) {
    string op_a, op_b, op_c;

    op_a = str::chunk(s);
    s = str::lstrip(str::sub(s, op_a.size()));

    op_b = str::chunk(s);
    s = str::lstrip(str::sub(s, op_b.size()));

    /* If s is empty and fill_third is true, use first operand as a filler.
     * In any other case, use the chunk of s.
     * The chunk of empty string will give us empty string and
     * it is a valid (and sometimes wanted) value to return.
     */
    op_c = (s.size() == 0 and fill_third ? op_a : str::chunk(s));

    return tuple<string, string, string>(op_a, op_b, op_c);
}
