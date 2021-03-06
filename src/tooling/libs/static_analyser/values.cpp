/*
 *  Copyright (C) 2018 Marek Marecki
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

#include <initializer_list>
#include <iostream>
#include <string>

#include <viua/tooling/errors/compile_time/errors.h>
#include <viua/tooling/libs/static_analyser/static_analyser.h>

namespace viua {
    namespace tooling {
        namespace libs {
            namespace static_analyser {
namespace values {
Value::Value(Value_type const t) : type_of_value{t}
{}

auto Value::type() const -> Value_type
{
    return type_of_value;
}

auto Value::clone(Value const& value, Value_wrapper::map_type& values)
    -> std::unique_ptr<Value>
{
    auto cloned = std::unique_ptr<Value>{};

    switch (value.type()) {
    case Value_type::Integer:
    {
        auto x = static_cast<Integer const&>(value);
        if (x.known()) {
            cloned = std::make_unique<Integer>(x.of());
        } else {
            cloned = std::make_unique<Integer>();
        }
        break;
    }
    case Value_type::Float:
    {
        cloned = std::make_unique<Float>();
        break;
    }
    case Value_type::Vector:
    {
        cloned = std::make_unique<Vector>(
            static_cast<Vector const&>(value).of().rebind(values));
        break;
    }
    case Value_type::String:
    {
        cloned = std::make_unique<String>();
        break;
    }
    case Value_type::Text:
    {
        cloned = std::make_unique<Text>();
        break;
    }
    case Value_type::Pointer:
    {
        cloned = std::make_unique<Pointer>(
            static_cast<Pointer const&>(value).of().rebind(values));
        break;
    }
    case Value_type::Boolean:
    {
        cloned = std::make_unique<Boolean>();
        break;
    }
    case Value_type::Bits:
    {
        cloned = std::make_unique<Bits>();
        break;
    }
    case Value_type::Closure:
    {
        cloned =
            std::make_unique<Closure>(static_cast<Closure const&>(value).of());
        break;
    }
    case Value_type::Function:
    {
        cloned = std::make_unique<Function>(
            static_cast<Function const&>(value).of());
        break;
    }
    case Value_type::Atom:
    {
        auto x = static_cast<Atom const&>(value);
        if (x.known()) {
            cloned = std::make_unique<Atom>(x.of());
        } else {
            cloned = std::make_unique<Atom>();
        }
        break;
    }
    case Value_type::Struct:
    {
        auto cloned_struct = std::make_unique<Struct>();
        auto x             = static_cast<Struct const&>(value);
        for (auto const& each : x.fields()) {
            cloned_struct->field(each.first, each.second.rebind(values));
        }
        break;
    }
    case Value_type::Pid:
    {
        cloned = std::make_unique<Pid>();
        break;
    }
    case Value_type::Value:
    {
        cloned = std::make_unique<Value>(Value_type::Value);
        break;
    }
    default:
        throw std::logic_error{"unhandled case"};
    }

    return cloned;
}

Integer::Integer() : Value{Value_type::Integer}
{}

Integer::Integer(int const x) : Value{Value_type::Integer}, n{x}
{}

auto Integer::known() const -> bool
{
    return n.has_value();
}

auto Integer::of() const -> int
{
    return n.value();
}
auto Integer::of(int const x) -> void
{
    n = x;
}

Float::Float() : Value{Value_type::Float}
{}

Vector::Vector(Value_wrapper v) : Value{Value_type::Vector}, contained_type{v}
{}

auto Vector::of() const -> Value_wrapper const&
{
    return contained_type;
}
auto Vector::of(Value_wrapper v) -> void
{
    contained_type = v;
}

Pointer::Pointer(Value_wrapper v)
        : Value{Value_type::Pointer}, contained_type{v}
{}

auto Pointer::of() const -> Value_wrapper const&
{
    return contained_type;
}
auto Pointer::of(Value_wrapper v) -> void
{
    contained_type = v;
}

String::String() : Value{Value_type::String}
{}

Text::Text() : Value{Value_type::Text}
{}

Boolean::Boolean() : Value{Value_type::Boolean}
{}

Bits::Bits() : Value{Value_type::Bits}
{}

Closure::Closure(std::string n) : Value{Value_type::Closure}, name{n}
{}

auto Closure::of() const -> std::string
{
    return name;
}

Function::Function(std::string n) : Value{Value_type::Function}, name{n}
{}

auto Function::of() const -> std::string
{
    return name;
}

Atom::Atom() : Value{Value_type::Atom}
{}

Atom::Atom(std::string s) : Value{Value_type::Atom}, value{s}
{}

auto Atom::known() const -> bool
{
    return value.has_value();
}

auto Atom::of() const -> std::string
{
    return value.value();
}

Struct::Struct() : Value{Value_type::Struct}
{}

auto Struct::fields() const -> decltype(known_fields) const&
{
    return known_fields;
}

auto Struct::field(std::string const& key) const -> std::optional<Value_wrapper>
{
    if (known_fields.count(key)) {
        return {known_fields.at(key)};
    }
    return {};
}

auto Struct::field(std::string key, Value_wrapper value) -> void
{
    known_fields.insert_or_assign(std::move(key), value);
}

Pid::Pid() : Value{Value_type::Pid}
{}

Value_wrapper::Value_wrapper(index_type const v, map_type& m) : i{v}, values{&m}
{}

Value_wrapper::Value_wrapper(Value_wrapper const& that)
        : i{that.i}, values{that.values}
{}

auto Value_wrapper::operator=(Value_wrapper const& that) -> Value_wrapper&
{
    i      = that.i;
    values = that.values;
    return *this;
}
auto Value_wrapper::operator=(Value_wrapper&& that) -> Value_wrapper&
{
    i      = that.i;
    values = that.values;
    return *this;
}

Value_wrapper::~Value_wrapper()
{}

auto Value_wrapper::Value_wrapper::value() const -> Value&
{
    return *(values->at(i));
}

auto Value_wrapper::Value_wrapper::value(std::unique_ptr<Value> v) -> void
{
    values->at(i) = std::move(v);
}

auto Value_wrapper::to_simple() const -> std::vector<Value_type>
{
    auto wrappers = std::vector<Value_wrapper const*>{this};

    while ((wrappers.back()->value().type() == Value_type::Vector)
           or (wrappers.back()->value().type() == Value_type::Pointer)) {
        if (wrappers.back()->value().type() == Value_type::Vector) {
            wrappers.push_back(
                &static_cast<Vector const&>(wrappers.back()->value()).of());
        } else {
            wrappers.push_back(
                &static_cast<Pointer const&>(wrappers.back()->value()).of());
        }
    }

    auto simple = std::vector<Value_type>{};

    for (auto const each : wrappers) {
        simple.push_back(each->value().type());
    }

    return simple;
}

auto Value_wrapper::index() const -> index_type
{
    return i;
}

auto Value_wrapper::rebind(map_type& to) const -> Value_wrapper
{
    return Value_wrapper{i, to};
}
}}}}}  // namespace viua::tooling::libs::static_analyser::values
