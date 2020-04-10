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

#ifndef VIUA_TYPES_EXCEPTION_H
#define VIUA_TYPES_EXCEPTION_H

#include <cstdint>
#include <string>
#include <vector>
#include <viua/bytecode/bytetypedef.h>
#include <viua/support/string.h>
#include <viua/types/value.h>


namespace viua { namespace types {
class Exception : public Value {
    /** Exception type.
     *
     *  Thrown when irrecoverable conditions are encountered
     *  during program execution.
     */
  protected:
    std::string cause;
    std::string detailed_type;

  public:
    static std::string const type_name;

    struct Throw_point {
        uint64_t const jump_base {0};
        uint64_t const offset {0};

        inline Throw_point(uint64_t const j, uint64_t const o)
            : jump_base{j}, offset{o}
        {}
    };
    std::vector<Throw_point> throw_points;

    std::string type() const override;
    std::string str() const override;
    std::string repr() const override;
    bool boolean() const override;

    std::unique_ptr<Value> copy() const override;

    virtual std::string what() const;
    virtual std::string etype() const;

    virtual auto add_throw_point(Throw_point const) const
        -> std::unique_ptr<Exception>;

    Exception(std::string s = "");
    Exception(std::string ts, std::string cs);
    Exception(std::vector<Throw_point>, std::string s = "");
    Exception(std::vector<Throw_point>, std::string ts, std::string cs);
};
}}  // namespace viua::types


#endif
