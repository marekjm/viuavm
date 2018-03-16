/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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

#ifndef VIUA_TYPES_REFERENCE_H
#define VIUA_TYPES_REFERENCE_H

#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <viua/types/value.h>

namespace viua {
    namespace types {
        class Reference : public Value {
            // FIXME maybe just use std::shared_ptr<> ?
            Value** pointer;
            uint64_t* counter;

          public:
            static const std::string type_name;

            std::string type() const override;
            std::string str() const override;
            std::string repr() const override;
            bool boolean() const override;

            std::vector<std::string> bases() const override;
            std::vector<std::string> inheritancechain() const override;

            std::unique_ptr<Value> copy() const override;

            virtual Value* points_to() const;
            virtual void rebind(Value*);
            virtual void rebind(std::unique_ptr<Value>);

            /*  This constructor is used internally by the Reference type to
             *  initialise copies of the reference.
             */
            Reference(Value** ptr, uint64_t* ctr);
            Reference(Value* ptr);
            virtual ~Reference();
        };
    }  // namespace types
}  // namespace viua


#endif
