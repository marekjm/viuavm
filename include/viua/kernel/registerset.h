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

#pragma once

#include <memory>
#include <vector>
#include <viua/bytecode/bytetypedef.h>
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
    void reset(std::unique_ptr<viua::types::Value>);
    bool empty() const;

    viua::types::Value* get();
    viua::types::Value* release();
    std::unique_ptr<viua::types::Value> give();

    void swap(Register&);

    mask_type set_mask(mask_type);
    mask_type get_mask() const;
    mask_type flag(mask_type);
    mask_type unflag(mask_type);
    bool is_flagged(mask_type) const;

    Register();
    Register(std::unique_ptr<viua::types::Value>);
    Register(Register&&);

    operator bool() const;
    auto operator=(Register &&) -> Register&;
    auto operator=(decltype(value)&&) -> Register&;
};

class Register_set {
    viua::internals::types::register_index registerset_size;
    std::vector<Register> registers;

  public:
    // basic access to registers
    void put(viua::internals::types::register_index,
             std::unique_ptr<viua::types::Value>);
    std::unique_ptr<viua::types::Value> pop(
        viua::internals::types::register_index);
    void set(viua::internals::types::register_index,
             std::unique_ptr<viua::types::Value>);
    viua::types::Value* get(viua::internals::types::register_index);
    viua::types::Value* at(viua::internals::types::register_index);

    Register* register_at(viua::internals::types::register_index);

    // register modifications
    void move(viua::internals::types::register_index,
              viua::internals::types::register_index);
    void swap(viua::internals::types::register_index,
              viua::internals::types::register_index);
    void empty(viua::internals::types::register_index);
    void free(viua::internals::types::register_index);

    // mask inspection and manipulation
    void flag(viua::internals::types::register_index, mask_type);
    void unflag(viua::internals::types::register_index, mask_type);
    void clear(viua::internals::types::register_index);
    bool isflagged(viua::internals::types::register_index, mask_type);
    void setmask(viua::internals::types::register_index, mask_type);
    mask_type getmask(viua::internals::types::register_index);

    void drop();
    inline viua::internals::types::register_index size() {
        return registerset_size;
    }

    std::unique_ptr<Register_set> copy();

    Register_set(viua::internals::types::register_index sz);
    ~Register_set();
};
}}  // namespace viua::kernel


#endif
