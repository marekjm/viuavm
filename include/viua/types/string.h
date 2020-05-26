/*
 *  Copyright (C) 2015-2017, 2020 Marek Marecki
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

#ifndef VIUA_TYPES_STRING_H
#define VIUA_TYPES_STRING_H

#include <memory>
#include <string>
#include <viua/kernel/frame.h>
#include <viua/kernel/registerset.h>
#include <viua/support/string.h>
#include <viua/types/integer.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>


namespace viua {
namespace process {
class Process;
}
namespace kernel {
class Kernel;
}
}  // namespace viua


namespace viua { namespace types {
class String : public Value {
    /** String type.
     *
     *  Designed to hold strings of bytes.
     *  Strings of bytes do not neccessarily represent human-readable text.
     *  They may represent just "strings of bytes".
     */
    std::string svalue;

  public:
    static std::string const type_name;

    std::string type() const override;
    std::string str() const override;
    std::string repr() const override;
    bool boolean() const override;

    std::unique_ptr<Value> copy() const override;

    auto operator==(String const&) const -> bool;

    auto value() const -> std::string const&;

    virtual void format(Frame*,
                        viua::kernel::Register_set*,
                        viua::kernel::Register_set*,
                        viua::process::Process*,
                        viua::kernel::Kernel*);

    String(std::string s = "");
    static auto make(std::string = "") -> std::unique_ptr<String>;
};
}}  // namespace viua::types


#endif
