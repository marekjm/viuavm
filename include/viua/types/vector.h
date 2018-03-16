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

#ifndef VIUA_TYPE_VECTOR_H
#define VIUA_TYPE_VECTOR_H

#pragma once

#include <string>
#include <vector>
#include <viua/types/value.h>


namespace viua {
    namespace types {
        class Vector : public Value {
            /** Vector type.
             */
            std::vector<std::unique_ptr<Value>> internal_object;

          public:
            static const std::string type_name;

            std::string type() const override;
            std::string str() const override;
            bool boolean() const override;
            std::unique_ptr<Value> copy() const override;

            std::vector<std::unique_ptr<Value>>& value();

            void insert(long int, std::unique_ptr<Value>);
            void push(std::unique_ptr<Value>);
            std::unique_ptr<Value> pop(long int);
            Value* at(long int);
            int len();

            Vector();
            Vector(const std::vector<Value*>& v);
            ~Vector();
        };
    }  // namespace types
}  // namespace viua


#endif
