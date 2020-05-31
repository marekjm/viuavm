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

#ifndef VIUA_REGISTERSET_H
#define VIUA_REGISTERSET_H

#include <memory>
#include <vector>

#include <viua/bytecode/codec.h>
#include <viua/types/value.h>

typedef uint8_t mask_type;


enum REGISTER_MASKS : mask_type {
    COPY_ON_WRITE = (1 << 0),
    MOVED         = (1 << 1),  // marks registers containing moved parameters
};


namespace viua { namespace kernel {
class Register {
    std::unique_ptr<viua::types::Value> value;
    mask_type mask;

  public:
    auto reset(std::unique_ptr<viua::types::Value>) -> void;
    auto empty() const -> bool;

    auto get() -> viua::types::Value*;
    auto get() const -> viua::types::Value const*;
    auto release() -> viua::types::Value*;
    auto give() -> std::unique_ptr<viua::types::Value>;

    auto swap(Register&) -> void;

    auto set_mask(mask_type) -> mask_type;
    auto get_mask() const -> mask_type;
    auto flag(mask_type) -> mask_type;
    auto unflag(mask_type) -> mask_type;
    auto is_flagged(mask_type) const -> bool;

    Register();
    Register(std::unique_ptr<viua::types::Value>);
    Register(Register const&) = delete;
    Register(Register&&);
    ~Register() = default;

    operator bool() const;
    auto operator=(Register const&) -> Register& = delete;
    auto operator                                =(Register &&) -> Register&;
    auto operator=(decltype(value)&&) -> Register&;
};

class Register_set {
  public:
    using size_type = viua::bytecode::codec::register_index_type;

    size_type registerset_size = 0;
    std::vector<Register> registers;

  public:
    auto put(size_type const, std::unique_ptr<viua::types::Value>) -> void;
    auto pop(size_type const) -> std::unique_ptr<viua::types::Value>;
    auto set(size_type const, std::unique_ptr<viua::types::Value>) -> void;
    auto get(size_type const) -> viua::types::Value*;
    auto get(size_type const) const -> viua::types::Value const*;
    auto at(size_type const) -> viua::types::Value*;
    auto at(size_type const) const -> viua::types::Value const*;

    auto register_at(size_type const) -> Register*;

    // register modifications
    auto move(size_type const, size_type const) -> void;
    auto swap(size_type const, size_type const) -> void;
    auto empty(size_type const) -> void;
    auto free(size_type const) -> void;

    // mask inspection and manipulation
    auto flag(size_type const, mask_type) -> void;
    auto unflag(size_type const, mask_type) -> void;
    auto clear(size_type const) -> void;
    auto is_flagged(size_type const, mask_type) const -> bool;
    auto set_mask(size_type const, mask_type) -> void;
    auto get_mask(size_type const) const -> mask_type;

    auto drop() -> void;
    inline auto size() const -> size_type
    {
        return registerset_size;
    }

    auto copy() const -> std::unique_ptr<Register_set>;

    Register_set(size_type const);
    Register_set(Register_set const&) = delete;
    Register_set(Register_set&&)      = delete;
    auto operator=(Register_set const&) -> Register_set& = delete;
    auto operator=(Register_set &&) -> Register_set& = delete;
    ~Register_set();
};
}}  // namespace viua::kernel


#endif
