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

#ifndef VIUA_TYPES_STRUCT_H
#define VIUA_TYPES_STRUCT_H

#pragma once

#include <string>
#include <map>
#include <vector>
#include <viua/types/type.h>


namespace viua {
    namespace types {
        class Struct: public Value {
            /** A generic object class.
             *
             *  This type is used internally inside the VM.
             */
            private:
                std::map<std::string, std::unique_ptr<Value>> attributes;

            public:
                static const std::string type_name;

                std::string type() const override;
                bool boolean() const override;

                std::string str() const override;
                std::string repr() const override;

                std::vector<std::string> bases() const override {
                    return std::vector<std::string>{"Value"};
                }
                std::vector<std::string> inheritancechain() const override {
                    return std::vector<std::string>{"Value"};
                }

                virtual void insert(const std::string& key, std::unique_ptr<Value> value);
                virtual std::unique_ptr<Value> remove(const std::string& key);
                virtual std::vector<std::string> keys() const;

                std::unique_ptr<Value> copy() const override;

                ~Struct() override = default;
        };
    }
}


#endif
