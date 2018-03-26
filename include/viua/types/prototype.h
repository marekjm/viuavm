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

#ifndef VIUA_TYPES_PROTOTYPE_H
#define VIUA_TYPES_PROTOTYPE_H

#pragma once

#include <map>
#include <viua/types/value.h>


namespace viua { namespace types {
class Prototype : public Value {
    /** A prototype of a type.
     *
     *  This type is used internally inside the VM.
     */

    std::string prototype_name;
    std::vector<std::string> ancestors;
    std::map<std::string, std::string> methods;
    std::vector<std::string> attributes;

  public:
    static const std::string type_name;

    std::string type() const override;
    bool boolean() const override;

    std::string str() const override;

    std::string get_type_name() const;
    std::vector<std::string> get_ancestors() const;

    // attach a function as a method to the prototype
    Prototype* attach(const std::string&, const std::string&);
    bool accepts(const std::string&) const;
    std::string resolves_to(const std::string&) const;

    // add an attribute to the prototype
    Prototype* add(const std::string&);

    // push a type to the inheritance chain of the prototype
    Prototype* derive(const std::string&);


    std::vector<std::string> bases() const override;
    std::vector<std::string> inheritancechain() const override;

    std::unique_ptr<Value> copy() const override;

    Prototype(const std::string& tn);
    virtual ~Prototype();
};
}}  // namespace viua::types


#endif
