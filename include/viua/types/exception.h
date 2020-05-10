/*
 *  Copyright (C) 2015, 2016, 2017, 2020 Marek Marecki
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

#ifndef VIUA_TYPES_EXCEPTION_H
#define VIUA_TYPES_EXCEPTION_H

#include <cstdint>
#include <string>
#include <vector>
#include <viua/bytecode/bytetypedef.h>
#include <viua/support/string.h>
#include <viua/types/value.h>


namespace viua { namespace types {
struct Exception : public Value {
    static std::string const type_name;

    /*
     * Tag is the value that is used to catch the exception in catch blocks. For
     * example, if the exception has tag 'Foo' it can be caught by the following
     * block:
     *
     *      catch "Foo" handle_foo
     */
    struct Tag {
        std::string tag;

        Tag(std::string t)
            : tag{std::move(t)}
        {}
    };
    std::string const tag {"Exception"};

    /*
     * Either of these may be specified, but not both at the same time.
     * The `description` string is just a placeholder for Text-typed values to
     * make it easier to throw exceptions from C++.
     */
    std::string const description {""};
    std::unique_ptr<Value> value { nullptr };

    struct Throw_point {
        uint64_t const jump_base {0};
        uint64_t const offset {0};
        std::string const name {};

        inline Throw_point(uint64_t const j, uint64_t const o)
            : jump_base{j}, offset{o}
        {}
        inline Throw_point(std::string n)
            : name{std::move(n)}
        {}
    };
    std::vector<Throw_point> throw_points;

    std::string type() const override;
    std::string str() const override;
    std::string repr() const override;
    bool boolean() const override;

    std::unique_ptr<Value> copy() const override;

    virtual auto what() const -> std::string;

    virtual auto add_throw_point(Throw_point const)
        -> void;

    Exception(Tag);
    Exception(std::string s = "");
    Exception(std::unique_ptr<Value>);
    Exception(Tag, std::string);
    Exception(Tag, std::unique_ptr<Value>);
    Exception(std::vector<Throw_point>, Tag);
};
}}  // namespace viua::types


#endif
