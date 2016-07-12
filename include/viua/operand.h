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

#ifndef VIUA_OPERAND_H
#define VIUA_OPERAND_H

#pragma once


#include <memory>
#include <string>
#include <tuple>
#include <viua/bytecode/bytetypedef.h>


// forward declaration since we need a pointer to CPU
class Process;
// and a pointer to Type
class Type;


namespace viua {
    namespace operand {
        class Operand {
            public:
                virtual Type* resolve(Process*) = 0;
                virtual ~Operand() {}
        };

        class Atom: public Operand {
                std::string atom;
            public:
                inline std::string get() const { return atom; }

                Type* resolve(Process*) override;

                Atom(const std::string& a): atom(a) {}
        };

        class RegisterIndex: public Operand {
                unsigned index;
            protected:
                RegisterIndex(): index(0) {}
            public:
                virtual unsigned get(Process*) const;
                Type* resolve(Process*) override;

                RegisterIndex(unsigned i): index(i) {}
        };

        class RegisterReference: public RegisterIndex {
                unsigned index;
            public:
                unsigned get(Process*) const override;
                Type* resolve(Process*) override;

                RegisterReference(unsigned i): index(i) {}
        };

        class Primitive: public Operand {
            public:
                Type* resolve(Process*) = 0;
        };

        class Int: public Primitive {
                int integer;
            public:
                Type* resolve(Process*) override;

                Int(int i): integer(i) {}
        };

        std::unique_ptr<viua::operand::Operand> extract(byte*& ip);
        std::string extractString(byte*& ip);
        unsigned getRegisterIndex(Operand*, Process*);
    }
}


#endif
