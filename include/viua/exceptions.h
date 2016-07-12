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

#include <string>
#include <sstream>
#include <vector>
#include <viua/types/exception.h>


class OutOfRangeException: public Exception {
    protected:
        std::string cause;

    public:
        std::string type() const { return "OutOfRangeException"; }
        OutOfRangeException(const std::string& s): cause(s) {}
};

class ArityException: public Exception {
        unsigned long got_arity;
        std::vector<decltype(got_arity)> valid_arities;
    public:
        std::string type() const override {
            return "ArityException";
        }

        std::string str() const override {
            std::ostringstream oss;
            oss << "got arity " << got_arity << ", expected one of {";
            for (decltype(valid_arities)::size_type i = 0; i < valid_arities.size(); ++i) {
                oss << valid_arities[i];
                if (i < (valid_arities.size()-1)) {
                    oss << ", ";
                }
            }
            oss << '}';
            return oss.str();
        }

        Type* copy() const override {
            return new ArityException(got_arity, valid_arities);
        }

        std::string what() const override {
            return str();
        }

        ArityException(decltype(got_arity) a, decltype(valid_arities) v): got_arity(a), valid_arities(v) {}
        ~ArityException() {}
};

class TypeException: public Exception {
        std::string expected;
        std::string got;
    public:
        std::string type() const override {
            return "TypeException";
        }

        std::string str() const override {
            std::ostringstream oss;
            oss << "expected " << expected << ", got " << got;
            return oss.str();
        }

        Type* copy() const override {
            return new TypeException(expected, got);
        }

        std::string what() const override {
            return str();
        }

        TypeException(decltype(expected) e, decltype(got) g): expected(e), got(g) {}
        ~TypeException() {}
};

class UnresolvedAtomException: public Exception {
        std::string atom;
    public:
        std::string type() const override {
            return "UnresolvedAtomException";
        }

        std::string str() const override {
            return ("atom '" + atom + "' could not be resolved");
        }

        Type* copy() const override {
            return new UnresolvedAtomException(atom);
        }

        std::string what() const override {
            return str();
        }

        UnresolvedAtomException(decltype(atom) a): atom(a) {}
        ~UnresolvedAtomException() {}
};

class OperandTypeException: public Exception {
    public:
        std::string type() const override {
            return "OperandTypeException";
        }

        std::string str() const override {
            return "invalid operand type";
        }

        Type* copy() const override {
            return new OperandTypeException();
        }

        std::string what() const override {
            return str();
        }
};

#endif
