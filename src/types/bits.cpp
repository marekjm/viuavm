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

#include <sstream>
#include <string>
#include <viua/types/bits.h>
#include <viua/types/exception.h>
using namespace std;
using namespace viua::types;

const string viua::types::Bits::type_name = "Bits";

string viua::types::Bits::type() const { return type_name; }

string viua::types::Bits::str() const {
    ostringstream oss;

    for (size_type i = underlying_array.size(); i; --i) {
        oss << underlying_array.at(i - 1);
    }

    return oss.str();
}

bool viua::types::Bits::boolean() const { return false; }

unique_ptr<viua::types::Value> viua::types::Bits::copy() const {
    return unique_ptr<viua::types::Value>{new Bits(underlying_array)};
}

auto viua::types::Bits::at(size_type i) const -> bool { return underlying_array.at(i); }

auto viua::types::Bits::set(size_type i, const bool value) -> bool {
    bool was = at(i);
    underlying_array.at(i) = value;
    return was;
}

auto viua::types::Bits::clear() -> void {
    for (auto i = underlying_array.size() - 1; i; --i) {
        set(i, false);
    }
}

auto viua::types::Bits::shl(size_type n) -> unique_ptr<Bits> {
    auto shifted = unique_ptr<Bits>{new Bits{n}};

    if (n >= underlying_array.size()) {
        clear();
        return shifted;
    }

    for (size_type i = 0; i < underlying_array.size(); ++i) {
        auto index_to_set = underlying_array.size() - 1 - i;
        auto index_of_value = underlying_array.size() - 1 - i - n;
        auto index_to_set_in_shifted = (n - 1 - i);

        if (index_of_value < underlying_array.size()) {
            if (index_to_set_in_shifted < n) {
                shifted->set(index_to_set_in_shifted, at(index_to_set));
            }
            set(index_to_set, at(index_of_value));
            set(index_of_value, false);
        } else {
            if (index_to_set_in_shifted < n) {
                shifted->set(index_to_set_in_shifted, at(index_to_set));
            }
            set(index_to_set, false);
        }
    }

    return shifted;
}

auto viua::types::Bits::shr(size_type n) -> unique_ptr<Bits> {
    auto shifted = unique_ptr<Bits>{new Bits{n}};

    if (n >= underlying_array.size()) {
        clear();
        return shifted;
    }

    for (size_type i = 0; i < underlying_array.size(); ++i) {
        auto index_to_set = i;
        auto index_of_value = i + n;
        auto index_to_set_in_shifted = i;

        if (index_of_value < underlying_array.size()) {
            if (index_to_set_in_shifted < n) {
                shifted->set(index_to_set_in_shifted, at(index_to_set));
            }
            set(index_to_set, at(index_of_value));
            set(index_of_value, false);
        } else {
            if (index_to_set_in_shifted < n) {
                shifted->set(index_to_set_in_shifted, at(index_to_set));
            }
            set(index_to_set, false);
        }
    }

    return shifted;
}

auto viua::types::Bits::ashl(size_type n) -> unique_ptr<Bits> {
    auto sign = at(underlying_array.size() - 1);
    auto shifted = shl(n);
    set(underlying_array.size() - 1, sign);
    return shifted;
}

auto viua::types::Bits::ashr(size_type n) -> unique_ptr<Bits> {
    auto sign = at(underlying_array.size() - 1);
    auto shifted = shr(n);
    set(underlying_array.size() - 1, sign);
    return shifted;
}

auto viua::types::Bits::rol(size_type n) -> void {
    auto shifted = shl(n);
    const auto offset = shifted->underlying_array.size();
    for (size_type i = 0; i < offset; ++i) {
        set(i, shifted->at(i));
    }
}

auto viua::types::Bits::ror(size_type n) -> void {
    auto shifted = shr(n);
    const auto offset = shifted->underlying_array.size();
    for (size_type i = 0; i < offset; ++i) {
        set(underlying_array.size() - offset + i, shifted->at(offset - 1 - i));
    }
}

auto viua::types::Bits::inverted() const -> unique_ptr<Bits> {
    auto result = unique_ptr<Bits>{new Bits{underlying_array.size()}};

    for (size_type i = 0; i < underlying_array.size(); ++i) {
        result->set(i, not at(i));
    }

    return result;
}

auto viua::types::Bits::operator|(const Bits& that) const -> unique_ptr<Bits> {
    unique_ptr<Bits> result = unique_ptr<Bits>{new Bits{underlying_array.size()}};

    for (size_type i = 0; i < underlying_array.size(); ++i) {
        result->set(i, (at(i) or that.at(i)));
    }

    return result;
}

auto viua::types::Bits::operator&(const Bits& that) const -> unique_ptr<Bits> {
    unique_ptr<Bits> result = unique_ptr<Bits>{new Bits{underlying_array.size()}};

    for (size_type i = 0; i < underlying_array.size(); ++i) {
        result->set(i, (at(i) and that.at(i)));
    }

    return result;
}

auto viua::types::Bits::operator^(const Bits& that) const -> unique_ptr<Bits> {
    unique_ptr<Bits> result = unique_ptr<Bits>{new Bits{underlying_array.size()}};

    for (size_type i = 0; i < underlying_array.size(); ++i) {
        result->set(i, (at(i) xor that.at(i)));
    }

    return result;
}

viua::types::Bits::Bits(const vector<bool>& bs) {
    underlying_array.reserve(bs.size());
    for (const auto each : bs) {
        underlying_array.push_back(each);
    }
}

viua::types::Bits::Bits(size_type i) {
    underlying_array.reserve(i);
    for (; i; --i) {
        underlying_array.push_back(false);
    }
}

viua::types::Bits::Bits(const size_type size, const uint8_t* source) {
    underlying_array.reserve(size * 8);
    for (auto i = size * 8; i; --i) {
        underlying_array.push_back(false);
    }
    const uint8_t one = 1;
    for (size_type byte_index = 0; byte_index < size; ++byte_index) {
        viua::internals::types::byte a_byte = *(source + byte_index);

        for (auto i = 0; i < 8; ++i) {
            auto mask = static_cast<decltype(one)>(one << i);
            underlying_array.at((size * 8) - 1 - ((byte_index * 8) + (7 - i))) = (a_byte & mask);
        }
    }
}
