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
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>


class Process;
class CPU;

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

        Type* copy() const {
            return new String(svalue);
        }

        std::string& value() { return svalue; }

        Integer* size();
        String* sub(int b = 0, int e = -1);
        String* add(String*);
        String* join(Vector*);

        virtual void stringify(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*);
        virtual void represent(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*);

        virtual void startswith(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*);
        virtual void endswith(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*);

        virtual void format(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*);
        virtual void substr(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*);
        virtual void concatenate(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*);
        virtual void join(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*);

        virtual void size(Frame*, RegisterSet*, RegisterSet*, Process*, CPU*);

        String(std::string s = ""): svalue(s) {}
};


#endif
