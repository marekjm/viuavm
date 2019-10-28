/*
 *  Copyright (C) 2017, 2019 Marek Marecki
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
#include <viua/exceptions.h>
#include <viua/support/string.h>
#include <viua/types/struct.h>
#include <viua/util/exceptions.h>


std::string const viua::types::Struct::type_name = "Struct";

std::string viua::types::Struct::type() const {
    return "Struct";
}

bool viua::types::Struct::boolean() const {
    return (not attributes.empty());
}

std::string viua::types::Struct::str() const {
    std::ostringstream oss;

    oss << '{';

    auto i = attributes.size();
    for (auto const& each : attributes) {
        oss << str::enquote(each.first, '\'') << ": " << each.second->repr();
        if (--i) {
            oss << ", ";
        }
    }

    oss << '}';

    return oss.str();
}

std::string viua::types::Struct::repr() const {
    return str();
}

void viua::types::Struct::insert(std::string const& key,
                                 std::unique_ptr<viua::types::Value> value) {
    attributes[key] = std::move(value);
}

std::unique_ptr<viua::types::Value> viua::types::Struct::remove(
    std::string const& key) {
    if (attributes.count(key) == 0) {
        using viua::util::exceptions::make_unique_exception;
        throw make_unique_exception<
            viua::runtime::exceptions::Invalid_field_access>();
    }

    auto value = std::move(attributes.at(key));
    attributes.erase(key);
    return value;
}

auto viua::types::Struct::at(std::string const& key) -> viua::types::Value* {
    if (attributes.count(key) == 0) {
        using viua::util::exceptions::make_unique_exception;
        throw make_unique_exception<
            viua::runtime::exceptions::Invalid_field_access>();
    }

    auto& value = attributes.at(key);
    return value.get();
}

auto viua::types::Struct::at(std::string const& key) const
    -> viua::types::Value const* {
    if (attributes.count(key) == 0) {
        using viua::util::exceptions::make_unique_exception;
        throw make_unique_exception<
            viua::runtime::exceptions::Invalid_field_access>();
    }

    auto& value = attributes.at(key);
    return value.get();
}

std::vector<std::string> viua::types::Struct::keys() const {
    auto ks = std::vector<std::string>{};
    for (auto const& each : attributes) {
        ks.push_back(each.first);
    }
    return ks;
}

std::unique_ptr<viua::types::Value> viua::types::Struct::copy() const {
    auto copied = std::make_unique<Struct>();
    for (auto const& each : attributes) {
        copied->insert(each.first, each.second->copy());
    }
    return copied;
}
