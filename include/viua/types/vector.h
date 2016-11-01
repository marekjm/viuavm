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

#ifndef VIUA_TYPE_VECTOR_H
#define VIUA_TYPE_VECTOR_H

#pragma once

#include <string>
#include <vector>
#include <viua/types/type.h>


namespace viua {
    namespace types {
        class Vector : public Type {
            /** Vector type.
             */
            std::vector<Type*> internal_object;

            public:
                std::string type() const {
                    return "Vector";
                }
                std::string str() const;
                bool boolean() const {
                    return internal_object.size() != 0;
                }

                Type* copy() const {
                    Vector* vec = new Vector();
                    for (unsigned i = 0; i < internal_object.size(); ++i) {
                        vec->push(internal_object[i]->copy());
                    }
                    return vec;
                }

                std::vector<Type*>& value() { return internal_object; }

                Type* insert(long int, Type*);
                Type* push(Type*);
                Type* pop(long int);
                Type* at(long int);
                int len();

                Vector() {}
                Vector(const std::vector<Type*>& v) {
                    for (unsigned i = 0; i < v.size(); ++i) {
                        internal_object.push_back(v[i]->copy());
                    }
                }
                ~Vector() {
                    while (internal_object.size()) {
                        delete internal_object.back();
                        internal_object.pop_back();
                    }
                }
        };
    }
}


#endif
