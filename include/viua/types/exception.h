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

#ifndef VIUA_TYPES_EXCEPTION_H
#define VIUA_TYPES_EXCEPTION_H

#pragma once

#include <string>
#include <viua/types/type.h>
#include <viua/support/string.h>


namespace viua {
    namespace types {
        class Exception : public Type {
            /** Exception type.
             *
             *  Thrown when irrecoverable conditions are encountered
             *  during program execution.
             */
            protected:
                std::string cause;
                std::string detailed_type;
            public:
                std::string type() const {
                    return "Exception";
                }
                std::string str() const {
                    return cause;
                }
                std::string repr() const {
                    return (etype() + ": " + str::enquote(cause));
                }
                bool boolean() const {
                    return true;
                }

                std::unique_ptr<Type> copy() const override {
                    return std::unique_ptr<viua::types::Type>{new Exception(cause)};
                }

                virtual std::string what() const;
                virtual std::string etype() const;

                Exception(std::string s = ""): cause(s), detailed_type("Exception") {}
                Exception(std::string ts, std::string cs): cause(cs), detailed_type(ts) {}
        };
    }
}


#endif
