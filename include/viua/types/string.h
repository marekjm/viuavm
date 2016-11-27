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

#ifndef VIUA_TYPES_STRING_H
#define VIUA_TYPES_STRING_H

#pragma once

#include <string>
#include <viua/types/type.h>
#include <viua/types/vector.h>
#include <viua/types/integer.h>
#include <viua/support/string.h>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>


namespace viua {
    namespace process {
        class Process;
    }
    namespace kernel {
        class Kernel;
    }
}


namespace viua {
    namespace types {
        class String : public Type {
            /** String type.
             *
             *  Designed to hold text.
             */
            std::string svalue;

            public:
                std::string type() const {
                    return "String";
                }
                std::string str() const {
                    return svalue;
                }
                std::string repr() const {
                    return str::enquote(svalue);
                }
                bool boolean() const {
                    return svalue.size() != 0;
                }

                std::unique_ptr<Type> copy() const override {
                    return std::unique_ptr<viua::types::Type>{new String(svalue)};
                }

                std::string& value() { return svalue; }

                Integer* size();
                String* sub(int b = 0, int e = -1);
                String* add(String*);
                String* join(Vector*);

                virtual void stringify(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*);
                virtual void represent(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*);

                virtual void startswith(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*);
                virtual void endswith(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*);

                virtual void format(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*);
                virtual void substr(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*);
                virtual void concatenate(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*);
                virtual void join(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*);

                virtual void size(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*);

                String(std::string s = ""): svalue(s) {}
        };
    }
}


#endif
