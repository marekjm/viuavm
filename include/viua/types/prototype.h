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

#ifndef VIUA_TYPES_PROTOTYPE_H
#define VIUA_TYPES_PROTOTYPE_H

#pragma once

#include <map>
#include <viua/types/type.h>


namespace viua {
    namespace types {
        class Prototype: public Type {
            /** A prototype of a type.
             *
             *  This type is used internally inside the VM.
             */

            std::string type_name;
            std::vector<std::string> ancestors;
            std::map<std::string, std::string> methods;
            std::vector<std::string> attributes;

            public:
                virtual std::string type() const override;
                virtual bool boolean() const override;

                virtual std::string str() const override;

                std::string getTypeName() const;
                std::vector<std::string> getAncestors() const;

                // attach a function as a method to the prototype
                Prototype* attach(const std::string&, const std::string&);
                bool accepts(const std::string&) const;
                std::string resolvesTo(const std::string&) const;

                // add an attribute to the prototype
                Prototype* add(const std::string&);

                // push a type to the inheritance chain of the prototype
                Prototype* derive(const std::string&);


                virtual std::vector<std::string> bases() const {
                    return std::vector<std::string>{"Type"};
                }
                virtual std::vector<std::string> inheritancechain() const {
                    return std::vector<std::string>{"Type"};
                }

                virtual std::unique_ptr<Type> copy() const override;

                Prototype(const std::string& tn): type_name(tn) {}
                virtual ~Prototype() {}
        };
    }
}


#endif
