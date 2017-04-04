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

#include <viua/types/text.h>
using namespace std;

namespace {
    const uint8_t UTF8_1ST_ROW = 0b00000000;
    const uint8_t UTF8_2ND_ROW = 0b11000000;
    const uint8_t UTF8_3RD_ROW = 0b11100000;
    const uint8_t UTF8_4TH_ROW = 0b11110000;
    const uint8_t UTF8_FILLING = 0b10000000;

    const uint8_t UTF8_1ST_ROW_NORMALISER = 0b10000000;
    const uint8_t UTF8_2ND_ROW_NORMALISER = 0b11100000;
    const uint8_t UTF8_3RD_ROW_NORMALISER = 0b11110000;
    const uint8_t UTF8_4TH_ROW_NORMALISER = 0b11111000;
    const uint8_t UTF8_FILLING_NORMALISER = 0b11000000;
}

static auto is_continuation_byte(uint8_t b) -> bool {
    return ((UTF8_FILLING_NORMALISER & b) == UTF8_FILLING);
}
auto viua::types::Text::parse(string s) -> decltype(text) {
    vector<Character> parsed_text;

    char ss[5];
    ss[4] = '\0';

    for (decltype(s)::size_type i = 0; i < s.size(); ++i) {
        const auto each { s.at(i) };
        ss[0] = each;
        ss[1] = '\0';
        ss[2] = '\0';
        ss[3] = '\0';

        if ((UTF8_1ST_ROW_NORMALISER & each) == UTF8_1ST_ROW) {
            // do nothing
        } else if ((UTF8_2ND_ROW_NORMALISER & each) == UTF8_2ND_ROW) {
            ss[1] = s.at(i+1);
        } else if ((UTF8_3RD_ROW_NORMALISER & each) == UTF8_3RD_ROW) {
            ss[1] = s.at(i+1);
            ss[2] = s.at(i+2);
        } else if ((UTF8_4TH_ROW_NORMALISER & each) == UTF8_4TH_ROW) {
            ss[1] = s.at(i+1);
            ss[2] = s.at(i+2);
            ss[3] = s.at(i+3);
        }

        if ((UTF8_1ST_ROW_NORMALISER & ss[0]) == UTF8_1ST_ROW) {
            // do nothing
        } else if ((UTF8_2ND_ROW_NORMALISER & ss[0]) == UTF8_2ND_ROW and is_continuation_byte(ss[1])) {
            ++i;
        } else if ((UTF8_3RD_ROW_NORMALISER & ss[0]) == UTF8_3RD_ROW and is_continuation_byte(ss[1]) and is_continuation_byte(ss[2])) {
            ++i;
            ++i;
        } else if ((UTF8_4TH_ROW_NORMALISER & ss[0]) == UTF8_4TH_ROW and is_continuation_byte(ss[1]) and is_continuation_byte(ss[2]) and is_continuation_byte(ss[3])) {
            ++i;
            ++i;
            ++i;
        } else {
            throw std::domain_error(s);
        }

        parsed_text.emplace_back(ss);
    }

    return parsed_text;
}

viua::types::Text::Text(string s): text(std::move(parse(s))) {
}
viua::types::Text::Text(vector<Character> s): text(std::move(s)) {
}

string viua::types::Text::type() const {
    return "Text";
}

string viua::types::Text::str() const {
    return "";
}

string viua::types::Text::repr() const {
    return "";
}

bool viua::types::Text::boolean() const {
    return false;
}

std::unique_ptr<viua::types::Type> viua::types::Text::copy() const {
    return std::unique_ptr<Type> { new Text(text) };
}
