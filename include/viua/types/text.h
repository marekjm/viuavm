/*
 *  Copyright (C) 2017 Marek Marecki
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

#ifndef VIUA_TYPES_TEXT_H
#define VIUA_TYPES_TEXT_H

#pragma once

#include <string>
#include <vector>
#include <viua/types/type.h>
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
        class Text : public Type {
            /**
             *  This type is designed to hold UTF-8 encoded text.
             *  Viua becomes tied to Unicode and the UTF-8 encoding.
             */
            public:
            using Character = std::string;

            private:
            std::vector<Character> text;

            auto parse(std::string) -> decltype(text);

            Text(std::vector<Character>);

            public:
                std::string type() const override;
                std::string str() const override;
                std::string repr() const override;
                bool boolean() const override;

                std::unique_ptr<Type> copy() const override;

                Text(std::string);
                ~Text() {}

                /* Integer* size(); */
                /* String* sub(int b = 0, int e = -1); */
                /* String* add(String*); */
                /* String* join(Vector*); */

                /* virtual void stringify(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*); */
                /* virtual void represent(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*); */

                /* virtual void startswith(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*); */
                /* virtual void endswith(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*); */

                /* virtual void format(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*); */
                /* virtual void substr(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*); */
                /* virtual void concatenate(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*); */
                /* virtual void join(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*); */

                /* virtual void size(Frame*, viua::kernel::RegisterSet*, viua::kernel::RegisterSet*, viua::process::Process*, viua::kernel::Kernel*); */

                /* String(std::string s = ""): svalue(s) {} */
        };
    }
}


#endif
