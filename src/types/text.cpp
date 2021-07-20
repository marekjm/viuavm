/*
 *  Copyright (C) 2017, 2020 Marek Marecki
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
#include <sstream>

#include <viua/support/string.h>
#include <viua/types/text.h>


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
}  // namespace

static auto is_continuation_byte(uint8_t b) -> bool
{
    return ((UTF8_FILLING_NORMALISER & b) == UTF8_FILLING);
}
static auto is_continuation_byte(char b) -> bool
{
    return is_continuation_byte(static_cast<uint8_t>(b));
}
auto viua::types::Text::parse(std::string s) -> decltype(text)
{
    auto parsed_text = std::vector<Character>{};

    char ss[5];
    ss[4] = '\0';

    for (decltype(s)::size_type i = 0; i < s.size(); ++i) {
        const auto each{s.at(i)};
        ss[0] = each;
        ss[1] = '\0';
        ss[2] = '\0';
        ss[3] = '\0';

        if ((UTF8_1ST_ROW_NORMALISER & each) == UTF8_1ST_ROW) {
            // do nothing
        } else if ((UTF8_2ND_ROW_NORMALISER & each) == UTF8_2ND_ROW) {
            ss[1] = s.at(i + 1);
        } else if ((UTF8_3RD_ROW_NORMALISER & each) == UTF8_3RD_ROW) {
            ss[1] = s.at(i + 1);
            ss[2] = s.at(i + 2);
        } else if ((UTF8_4TH_ROW_NORMALISER & each) == UTF8_4TH_ROW) {
            ss[1] = s.at(i + 1);
            ss[2] = s.at(i + 2);
            ss[3] = s.at(i + 3);
        }

        if ((UTF8_1ST_ROW_NORMALISER & ss[0]) == UTF8_1ST_ROW) {
            // do nothing
        } else if ((UTF8_2ND_ROW_NORMALISER & ss[0]) == UTF8_2ND_ROW
                   and is_continuation_byte(ss[1])) {
            ++i;
        } else if ((UTF8_3RD_ROW_NORMALISER & ss[0]) == UTF8_3RD_ROW
                   and is_continuation_byte(ss[1])
                   and is_continuation_byte(ss[2])) {
            ++i;
            ++i;
        } else if ((UTF8_4TH_ROW_NORMALISER & ss[0]) == UTF8_4TH_ROW
                   and is_continuation_byte(ss[1])
                   and is_continuation_byte(ss[2])
                   and is_continuation_byte(ss[3])) {
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

viua::types::Text::Text(std::string s) : text{parse(s)}, text_str{std::move(s)}
{}
viua::types::Text::Text(std::vector<Character> s)
        : text(std::move(s)), text_str{str()}
{}
viua::types::Text::Text(Text&& s)
        : text{std::move(s.text)}, text_str{std::move(s.text_str)}
{}

auto viua::types::Text::type() const -> std::string
{
    return type_name;
}

auto viua::types::Text::str() const -> std::string
{
    auto oss = std::ostringstream{};
    for (auto const& each : text) {
        oss << each;
    }
    return oss.str();
}

auto viua::types::Text::repr() const -> std::string
{
    return str::enquote(data());
}

auto viua::types::Text::boolean() const -> bool
{
    return false;
}

auto viua::types::Text::copy() const -> std::unique_ptr<viua::types::Value>
{
    return std::make_unique<Text>(text);
}

auto viua::types::Text::operator==(viua::types::Text const& other) const -> bool
{
    return (text == other.text);
}

auto viua::types::Text::operator+(viua::types::Text const& other) const -> Text
{
    auto copied = decltype(text){};
    for (size_type i = 0; i < size(); ++i) {
        copied.push_back(text.at(i));
    }
    for (size_type i = 0; i < other.size(); ++i) {
        copied.push_back(other.at(i));
    }
    return copied;
}

auto viua::types::Text::at(const size_type i) const -> Character
{
    return text.at(i);
}

auto viua::types::Text::signed_size() const -> int64_t
{
    return static_cast<int64_t>(text.size());
}
auto viua::types::Text::size() const -> size_type
{
    return text.size();
}


auto viua::types::Text::sub(size_type first_index, size_type last_index) const
    -> decltype(text)
{
    auto copied = decltype(text){};
    for (size_type i = first_index; i < size() and i < last_index; ++i) {
        copied.push_back(text.at(i));
    }
    return copied;
}
auto viua::types::Text::sub(size_type first_index) const -> decltype(text)
{
    return sub(first_index, text.size());
}


auto viua::types::Text::common_prefix(Text const& other) const -> size_type
{
    auto length_of_common_prefix = size_type{0};
    auto limit                   = std::max(size(), other.size());

    while (length_of_common_prefix < limit
           and text.at(length_of_common_prefix)
                   == other.at(length_of_common_prefix)) {
        ++length_of_common_prefix;
    }

    return length_of_common_prefix;
}
auto viua::types::Text::common_suffix(Text const& other) const -> size_type
{
    auto length_of_common_suffix = size_type{0};

    auto this_index  = size_type{size() - 1};
    auto other_index = size_type{other.size() - 1};

    while ((this_index and other_index)
           and text.at(this_index) == other.at(other_index)) {
        ++length_of_common_suffix;
        --this_index;
        --other_index;
    }

    return length_of_common_suffix;
}

auto viua::types::Text::data() const -> std::string const&
{
    return text_str;
}
