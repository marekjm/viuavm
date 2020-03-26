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
    Register(Register&&);

    operator bool() const;
    auto operator=(Register &&) -> Register&;
    auto operator=(decltype(value)&&) -> Register&;
};

class Register_set {
    viua::bytecode::codec::register_index_type registerset_size;
    std::vector<Register> registers;

  public:
    // basic access to registers
    void put(viua::bytecode::codec::register_index_type,
             std::unique_ptr<viua::types::Value>);
    std::unique_ptr<viua::types::Value> pop(
        viua::bytecode::codec::register_index_type);
    void set(viua::bytecode::codec::register_index_type,
             std::unique_ptr<viua::types::Value>);
    viua::types::Value* get(viua::bytecode::codec::register_index_type);
    viua::types::Value* at(viua::bytecode::codec::register_index_type);

    Register* register_at(viua::bytecode::codec::register_index_type);

    // register modifications
    void move(viua::bytecode::codec::register_index_type,
              viua::bytecode::codec::register_index_type);
    void swap(viua::bytecode::codec::register_index_type,
              viua::bytecode::codec::register_index_type);
    void empty(viua::bytecode::codec::register_index_type);
    void free(viua::bytecode::codec::register_index_type);

    // mask inspection and manipulation
    void flag(viua::bytecode::codec::register_index_type, mask_type);
    void unflag(viua::bytecode::codec::register_index_type, mask_type);
    void clear(viua::bytecode::codec::register_index_type);
    bool isflagged(viua::bytecode::codec::register_index_type, mask_type);
    void setmask(viua::bytecode::codec::register_index_type, mask_type);
    mask_type getmask(viua::bytecode::codec::register_index_type);

    void drop();
    inline viua::bytecode::codec::register_index_type size()
    {
        return registerset_size;
    }

    std::unique_ptr<Register_set> copy();

    Register_set(viua::bytecode::codec::register_index_type sz);
    ~Register_set();
};
}}  // namespace viua::kernel


#endif
