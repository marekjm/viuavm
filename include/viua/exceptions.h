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

#ifndef VIUA_EXCEPTIONS_H
#define VIUA_EXCEPTIONS_H

#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <viua/types/exception.h>
#include <viua/util/exceptions.h>


class Out_of_range_exception : public viua::types::Exception {
  public:
    std::string type() const {
        return "Out_of_range_exception";
    }
    Out_of_range_exception(std::string const& s) : viua::types::Exception(s) {}
};

class Arity_exception : public viua::types::Exception {
    viua::internals::types::register_index got_arity;
    std::vector<decltype(got_arity)> valid_arities;

  public:
    std::string type() const override {
        return "Arity_exception";
    }

    std::string str() const override {
        auto oss = std::ostringstream{};
        oss << "got arity " << got_arity << ", expected one of {";
        for (decltype(valid_arities)::size_type i = 0; i < valid_arities.size();
             ++i) {
            oss << valid_arities[i];
            if (i < (valid_arities.size() - 1)) {
                oss << ", ";
            }
        }
        oss << '}';
        return oss.str();
    }

    std::unique_ptr<Value> copy() const override {
        return viua::util::exceptions::make_unique_exception<Arity_exception>(
            got_arity, valid_arities);
    }

    std::string what() const override {
        return str();
    }

    Arity_exception(decltype(got_arity) a, decltype(valid_arities) v)
            : got_arity(a), valid_arities(v) {}
    ~Arity_exception() {}
};

class Type_exception : public viua::types::Exception {
    std::string expected;
    std::string got;

  public:
    std::string type() const override {
        return "Type_exception";
    }

    std::string str() const override {
        auto oss = std::ostringstream{};
        oss << "expected " << expected << ", got " << got;
        return oss.str();
    }

    std::unique_ptr<Value> copy() const override {
        return viua::util::exceptions::make_unique_exception<Type_exception>(
            expected, got);
    }

    std::string what() const override {
        return str();
    }

    Type_exception(decltype(expected) e, decltype(got) g)
            : expected(e), got(g) {}
    ~Type_exception() {}
};

class Unresolved_atom_exception : public viua::types::Exception {
    std::string atom;

  public:
    std::string type() const override {
        return "Unresolved_atom_exception";
    }

    std::string str() const override {
        return ("atom '" + atom + "' could not be resolved");
    }

    std::unique_ptr<Value> copy() const override {
        return viua::util::exceptions::make_unique_exception<
            Unresolved_atom_exception>(atom);
    }

    std::string what() const override {
        return str();
    }

    Unresolved_atom_exception(decltype(atom) a) : atom(a) {}
    ~Unresolved_atom_exception() {}
};

class Operand_type_exception : public viua::types::Exception {
  public:
    std::string type() const override {
        return "Operand_type_exception";
    }

    std::string str() const override {
        return "invalid operand type";
    }

    std::unique_ptr<Value> copy() const override {
        return viua::util::exceptions::make_unique_exception<
            Operand_type_exception>();
    }

    std::string what() const override {
        return str();
    }
};

namespace viua { namespace runtime { namespace exceptions {
class Zero_division : public viua::types::Exception {
  public:
    std::string type() const override {
        return "Zero_division";
    }

    std::string str() const override {
        return "zero division";
    }

    std::unique_ptr<Value> copy() const override {
        return viua::util::exceptions::make_unique_exception<Zero_division>();
    }

    std::string what() const override {
        return str();
    }
};

class Invalid_field_access : public viua::types::Exception {
  public:
    std::string type() const override {
        return "Invalid_field_access";
    }

    std::string str() const override {
        return "invalid field access";
    }

    std::unique_ptr<Value> copy() const override {
        return viua::util::exceptions::make_unique_exception<
            Invalid_field_access>();
    }

    std::string what() const override {
        return str();
    }
};
}}}  // namespace viua::runtime::exceptions

#endif
