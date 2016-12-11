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

#ifndef VIUA_TYPE_FUNCTION_H
#define VIUA_TYPE_FUNCTION_H

#pragma once

#include <string>
#include <viua/bytecode/bytetypedef.h>
#include <viua/kernel/registerset.h>
#include <viua/types/type.h>


namespace viua {
    namespace types {
        class Function : public Type {
            /** Type representing a function.
             */
            public:
                std::string function_name;

                virtual std::string type() const override;
                virtual std::string str() const override;
                virtual std::string repr() const override;

                virtual bool boolean() const override;

                virtual std::unique_ptr<Type> copy() const override;

                virtual std::string name() const;

                // FIXME: implement real dtor
                Function(const std::string& = "");
                virtual ~Function();
        };
    }
}


#endif
