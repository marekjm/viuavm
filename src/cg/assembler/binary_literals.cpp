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

#include <viua/cg/assembler/assembler.h>
using namespace std;


auto assembler::operands::normalise_binary_literal(const string s) -> string {
    ostringstream oss;

    if (s.size() > 2 and s.at(1) == 'b') {
        // FIXME create InternalError class
        throw("internal error: invalid binary literal: cannot normalise literals with '0b' prefix: " + s);
    }

    auto n = 0;
    while ((s.size() + n) % 8 != 0) {
        oss << '0';
        ++n;
    }
    oss << s;

    return oss.str();
}
static auto strip_leading_zeroes(const string s) -> string {
    auto leading_zeroes = decltype(s)::size_type{0};
    while (leading_zeroes < s.size() and s.at(leading_zeroes) == '0') {
        ++leading_zeroes;
    }
    return s.substr(leading_zeroes);
}
auto assembler::operands::octal_to_binary_literal(const string s) -> string {
    ostringstream oss;
    const static map<const char, const string> lookup = {
        {
            '0', "000",
        },
        {
            '1', "001",
        },
        {
            '2', "010",
        },
        {
            '3', "011",
        },
        {
            '4', "100",
        },
        {
            '5', "101",
        },
        {
            '6', "110",
        },
        {
            '7', "111",
        },
    };
    for (const auto c : s.substr(2)) {
        oss << lookup.at(c);
    }
    return strip_leading_zeroes(oss.str());
}
auto assembler::operands::hexadecimal_to_binary_literal(const string s) -> string {
    ostringstream oss;
    const static map<const char, const string> lookup = {
        {
            '0', "0000",
        },
        {
            '1', "0001",
        },
        {
            '2', "0010",
        },
        {
            '3', "0011",
        },
        {
            '4', "0100",
        },
        {
            '5', "0101",
        },
        {
            '6', "0110",
        },
        {
            '7', "0111",
        },
        {
            '8', "1000",
        },
        {
            '9', "1001",
        },
        {
            'a', "1010",
        },
        {
            'b', "1011",
        },
        {
            'c', "1100",
        },
        {
            'd', "1101",
        },
        {
            'e', "1110",
        },
        {
            'f', "1111",
        },
    };
    for (const auto c : s.substr(2)) {
        oss << lookup.at(c);
    }
    return strip_leading_zeroes(oss.str());
}
