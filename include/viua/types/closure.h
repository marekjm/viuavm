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

#ifndef VIUA_TYPE_CLOSURE_H
#define VIUA_TYPE_CLOSURE_H

#include <memory>
#include <string>

#include <viua/bytecode/bytetypedef.h>
#include <viua/kernel/registerset.h>
#include <viua/types/function.h>


namespace viua { namespace types {
class Closure : public Function {
    std::unique_ptr<viua::kernel::Register_set> local_register_set;

  public:
    constexpr static auto type_name = "Closure";

    auto type() const -> std::string override;
    auto str() const -> std::string override;
    auto repr() const -> std::string override;

    auto boolean() const -> bool override;

    auto copy() const -> std::unique_ptr<Value> override;

    auto name() const -> std::string override;
    auto rs() const -> viua::kernel::Register_set*;
    auto release() -> viua::kernel::Register_set*;
    auto give() -> std::unique_ptr<viua::kernel::Register_set>;
    auto empty() const -> bool;
    auto set(viua::bytecode::codec::register_index_type const,
             std::unique_ptr<viua::types::Value>) -> void;

    Closure(std::string const&, std::unique_ptr<viua::kernel::Register_set>);
    virtual ~Closure();
};
}}  // namespace viua::types


#endif
