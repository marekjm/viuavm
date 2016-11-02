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

#ifndef VIUA_TYPES_POINTER_H
#define VIUA_TYPES_POINTER_H

#pragma once

#include <vector>
#include <viua/types/type.h>
#include <viua/kernel/frame.h>


class Process;
namespace viua {
    namespace kernel {
        class Kernel;
    }
}


namespace viua {
    namespace types {
        class Pointer: public Type {
                Type* points_to;
                bool valid;

                void attach();
                void detach();
            public:
                void invalidate(Type* t);
                bool expired();
                void reset(Type* t);
                Type* to();

                virtual void expired(Frame*, RegisterSet*, RegisterSet*, Process*, viua::kernel::Kernel*);

                std::string str() const override;

                std::string type() const override;
                bool boolean() const override;

                std::vector<std::string> bases() const override {
                    return std::vector<std::string>{"Type"};
                }
                std::vector<std::string> inheritancechain() const override {
                    return std::vector<std::string>{"Type"};
                }

                Type* copy() const override;

                Pointer();
                Pointer(Type* t);
                virtual ~Pointer();
        };
    }
}


#endif
