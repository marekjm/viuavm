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


namespace viua {
    namespace kernel {
        class Kernel;
    }

    namespace types {
        class Pointer: public Type {
                Type* points_to;
                bool valid;
                /*
                 *  Pointer of origin is a parallelism-safety token.
                 *  Viua asserts that pointers can be dereferenced only
                 *  inside the process that spawned them -- otherwise
                 *  there is no way to ensure that the pointer is still valid
                 *  without sophisticated sycnhronisation schemes.
                 *  Also, since the VM employs shared-nothing concurrency,
                 *  pointer dereferences outside of the process that taken the
                 *  pointer should be illegal by definition (even if the access
                 *  could be made safe).
                 */
                const viua::process::Process *process_of_origin;

                void attach();
                void detach();
            public:
                void invalidate(Type* t);
                bool expired();
                auto authenticate(const viua::process::Process*) -> void;
                void reset(Type* t);
                Type* to(const viua::process::Process*);

                virtual void expired(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*);

                std::string str() const override;

                std::string type() const override;
                bool boolean() const override;

                std::vector<std::string> bases() const override {
                    return std::vector<std::string>{"Type"};
                }
                std::vector<std::string> inheritancechain() const override {
                    return std::vector<std::string>{"Type"};
                }

                std::unique_ptr<Type> copy() const override;

                Pointer();
                Pointer(Type* t, const viua::process::Process*);
                virtual ~Pointer();
        };
    }
}


#endif
