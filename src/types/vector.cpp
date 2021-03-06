/*
 *  Copyright (C) 2015-2017, 2020 Marek Marecki
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

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <viua/types/exception.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>


void viua::types::Vector::insert(long int index,
                                 std::unique_ptr<viua::types::Value> object)
{
    long offset = 0;

    // FIXME: REFACTORING: move bounds-checking to a separate function
    if (index > 0
        and static_cast<decltype(internal_object)::size_type>(index)
                > internal_object.size()) {
        using viua::types::Exception;
        throw std::make_unique<Exception>(
            Exception::Tag{"Out_of_bounds"},
            ("positive index " + std::to_string(index) + " with size "
             + std::to_string(internal_object.size())));
    } else if (index < 0
               and static_cast<decltype(internal_object)::size_type>(-index)
                       > internal_object.size()) {
        using viua::types::Exception;
        throw std::make_unique<Exception>(
            Exception::Tag{"Out_of_bounds"},
            ("negative index " + std::to_string(index) + " with size "
             + std::to_string(internal_object.size())));
    }
    if (index < 0) {
        offset = (static_cast<decltype(index)>(internal_object.size()) + index);
    } else {
        offset = index;
    }

    auto it = (internal_object.begin() + offset);
    internal_object.insert(it, std::move(object));
}

void viua::types::Vector::push(std::unique_ptr<viua::types::Value> object)
{
    internal_object.emplace_back(std::move(object));
}

std::unique_ptr<viua::types::Value> viua::types::Vector::pop(long int index)
{
    long offset = 0;

    // FIXME: REFACTORING: move bounds-checking to a separate function
    if (internal_object.size() == 0) {
        using viua::types::Exception;
        throw std::make_unique<Exception>(
            Exception::Tag{"Out_of_bounds"},
            ("positive index " + std::to_string(index) + " with empty vector"));
    } else if (index > 0
               and static_cast<decltype(internal_object)::size_type>(index)
                       >= internal_object.size()) {
        using viua::types::Exception;
        throw std::make_unique<Exception>(
            Exception::Tag{"Out_of_bounds"},
            ("positive index " + std::to_string(index) + " with size "
             + std::to_string(internal_object.size())));
    } else if (index < 0
               and static_cast<decltype(internal_object)::size_type>(-index)
                       > internal_object.size()) {
        using viua::types::Exception;
        throw std::make_unique<Exception>(
            Exception::Tag{"Out_of_bounds"},
            ("negative index " + std::to_string(index) + " with size "
             + std::to_string(internal_object.size())));
    }

    if (index < 0) {
        offset = (static_cast<decltype(index)>(internal_object.size()) + index);
    } else {
        offset = index;
    }

    auto it = (internal_object.begin() + offset);
    std::unique_ptr<viua::types::Value> object = std::move(*it);
    internal_object.erase(it);
    return object;
}

viua::types::Value* viua::types::Vector::at(long int index)
{
    long offset = 0;

    // FIXME: REFACTORING: move bounds-checking to a separate function
    if (internal_object.size() == 0) {
        using viua::types::Exception;
        throw std::make_unique<Exception>(
            Exception::Tag{"Out_of_bounds"},
            ("positive index " + std::to_string(index) + " with empty vector"));
    } else if (index > 0
               and static_cast<decltype(internal_object)::size_type>(index)
                       >= internal_object.size()) {
        using viua::types::Exception;
        throw std::make_unique<Exception>(
            Exception::Tag{"Out_of_bounds"},
            ("positive index " + std::to_string(index) + " with size "
             + std::to_string(internal_object.size())));
    } else if (index < 0
               and static_cast<decltype(internal_object)::size_type>(-index)
                       > internal_object.size()) {
        using viua::types::Exception;
        throw std::make_unique<Exception>(
            Exception::Tag{"Out_of_bounds"},
            ("negative index " + std::to_string(index) + " with size "
             + std::to_string(internal_object.size())));
    }

    if (index < 0) {
        offset = (static_cast<decltype(index)>(internal_object.size()) + index);
    } else {
        offset = index;
    }

    auto it = (internal_object.begin() + offset);
    return it->get();
}

int viua::types::Vector::len()
{
    // FIXME: should return unsigned
    // FIXME: VM does not have unsigned integer type so return value has
    // to be converted to signed integer
    return static_cast<int>(internal_object.size());
}

std::string viua::types::Vector::type() const
{
    return type_name;
}

std::string viua::types::Vector::str() const
{
    std::ostringstream oss;
    oss << "[";
    for (decltype(internal_object)::size_type i = 0; i < internal_object.size();
         ++i) {
        oss << internal_object[i]->repr()
            << (i < internal_object.size() - 1 ? ", " : "");
    }
    oss << "]";
    return oss.str();
}

bool viua::types::Vector::boolean() const
{
    return internal_object.size() != 0;
}

std::unique_ptr<viua::types::Value> viua::types::Vector::copy() const
{
    auto v = std::make_unique<Vector>();
    for (auto i = size_t{0}; i < internal_object.size(); ++i) {
        v->push(internal_object[i]->copy());
    }
    return v;
}

std::vector<std::unique_ptr<viua::types::Value>>& viua::types::Vector::value()
{
    return internal_object;
}

std::vector<std::unique_ptr<viua::types::Value>> const& viua::types::Vector::
    value() const
{
    return internal_object;
}

auto viua::types::Vector::expire() -> void
{
    for (auto& each : internal_object) {
        each->expire();
    }
}

viua::types::Vector::Vector()
{}
viua::types::Vector::Vector(const std::vector<viua::types::Value*>& v)
{
    for (unsigned i = 0; i < v.size(); ++i) {
        internal_object.push_back(v[i]->copy());
    }
}
viua::types::Vector::~Vector()
{}
