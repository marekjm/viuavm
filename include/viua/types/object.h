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

#ifndef VIUA_TYPES_OBJECT_H
#define VIUA_TYPES_OBJECT_H

#pragma once

#include <map>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/types/value.h>


namespace viua { namespace types {
class Object : public Value {
    /** A generic object class.
     *
     *  This type is used internally inside the VM.
     */
  private:
    std::string object_type_name;
    std::map<std::string, std::unique_ptr<Value>> attributes;

  public:
    static const std::string type_name;

    std::string type() const override;
    bool boolean() const override;

    std::string str() const override;

    void insert(const std::string& key, std::unique_ptr<Value> value);
    std::unique_ptr<Value> remove(const std::string& key);

    void set(const std::string&, std::unique_ptr<Value>);
    inline Value* at(const std::string& s) {
        return attributes.at(s).get();
    }

    virtual std::unique_ptr<Value> copy() const override;

    Object(const std::string& tn);
    virtual ~Object();
};
}}  // namespace viua::types


#endif
