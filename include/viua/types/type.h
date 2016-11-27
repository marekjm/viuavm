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

#ifndef VIUA_TYPES_TYPE_H
#define VIUA_TYPES_TYPE_H

#pragma once

#include <memory>
#include <string>
#include <sstream>
#include <vector>


namespace viua {
    namespace types {
        class Pointer;

        class Type {
            /** Base class for all derived types.
             *  Viua uses an object-based hierarchy to allow easier storage in registers and
             *  to take advantage of C++ polymorphism.
             *
             *  Instead of void* Viua holds Type* so when registers are delete'ed proper destructor
             *  is always called.
             */
            friend class Pointer;
            std::vector<Pointer*> pointers;

            public:
                /** Basic interface of a Type.
                 *
                 *  Derived objects are expected to override this methods, but in case they do not
                 *  Type provides safe defaults.
                 */
                virtual std::string type() const {
                    /*  Basic type is just `Type`.
                     */
                    return "Type";
                }
                virtual std::string str() const {
                    /*  By default, Viua provides string output a la Python.
                     *  This means - type of the object and its location in memory.
                     */
                    std::ostringstream s;
                    s << "<'" << type() << "' object at " << this << ">";
                    return s.str();
                }
                virtual std::string repr() const {
                    /** This is akin to Python's repr.
                     *  String returned by this method can be used to represent the value in source code.
                     */
                    return str();
                }
                virtual bool boolean() const {
                    /*  Boolean defaults to false.
                     *  This is because in if, loops etc. we will NOT execute code depending on unknown state.
                     *  If a derived object overrides this method it is free to return true as it sees fit, but
                     *  the default is to NOT carry any actions.
                     */
                    return false;
                }

                virtual std::unique_ptr<Pointer> pointer();

                virtual std::vector<std::string> bases() const {
                    return std::vector<std::string>{"Type"};
                }
                virtual std::vector<std::string> inheritancechain() const {
                    return std::vector<std::string>{"Type"};
                }

                virtual std::unique_ptr<Type> copy() const = 0;

                // We need to construct and destroy our basic object.
                Type() {}
                virtual ~Type();
        };
    }
}


#endif
