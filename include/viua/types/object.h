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

#ifndef VIUA_TYPES_OBJECT_H
#define VIUA_TYPES_OBJECT_H

#pragma once

#include <map>
#include <viua/cpu/frame.h>
#include <viua/cpu/registerset.h>
#include <viua/types/type.h>


class Object: public Type {
    /** A generic object class.
     *
     *  This type is used internally inside the VM.
     */
    private:
        std::string type_name;
        std::map<std::string, Type*> attributes;

    public:
        virtual std::string type() const;
        virtual bool boolean() const;

        virtual std::vector<std::string> bases() const {
            return std::vector<std::string>{"Type"};
        }
        virtual std::vector<std::string> inheritancechain() const {
            return std::vector<std::string>{"Type"};
        }

        virtual void insert(const std::string& key, Type* value);
        virtual Type* remove(const std::string& key);

        void set(const std::string&, Type*);
        inline Type* at(const std::string& s) { return attributes.at(s); }

        virtual Type* copy() const;

        Object(const std::string& tn);
        virtual ~Object();
};


#endif
