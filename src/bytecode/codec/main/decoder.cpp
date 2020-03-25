/*
 *  Copyright (C) 2019, 2020 Marek Marecki
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

#include <endian.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <viua/bytecode/codec/main.h>
#include <viua/bytecode/operand_types.h>
#include <viua/util/memory.h>

namespace viua { namespace bytecode { namespace codec { namespace main {
auto get_operand_type(uint8_t const* const addr) -> OperandType
{
    return static_cast<OperandType>(*addr);
}
auto is_void(uint8_t const* const addr) -> bool
{
    return (get_operand_type(addr) == OT_VOID);
}

// FIXME add error checking to all functions
auto Decoder::decode_register(uint8_t const* addr) const
    -> std::pair<uint8_t const*, Register_access>
{
    auto access_type = Access_specifier::Direct;
    {
        auto const ot = static_cast<OperandType>(*addr);
        ++addr;
        if (ot == OT_REGISTER_INDEX) {
            access_type = Access_specifier::Direct;
        } else if (ot == OT_POINTER) {
            access_type = Access_specifier::Pointer_dereference;
        } else if (ot == OT_REGISTER_REFERENCE) {
            access_type = Access_specifier::Register_indirect;
        }
    }

    auto register_set = static_cast<Register_set>(*addr);
    ++addr;

    auto index = register_index_type{0};
    {
        std::memcpy(&index, addr, sizeof(index));
        addr += sizeof(index);
    }

    return {addr, Register_access{register_set, be16toh(index), access_type}};
}

auto Decoder::decode_string(uint8_t const* addr) const
    -> std::pair<uint8_t const*, std::string>
{
    auto s = std::string{reinterpret_cast<char const*>(addr)};

    return {addr + s.size() + 1, std::move(s)};
}

auto Decoder::decode_bits_string(uint8_t const* addr) const
    -> std::pair<uint8_t const*, std::vector<uint8_t>>
{
    ++addr;  // FIXME skip operand type, assume it's correct (OT_BITS)

    auto const size = viua::util::memory::load_aligned<uint64_t>(addr);
    addr += sizeof(decltype(size));

    auto data = std::vector<uint8_t>{};
    data.reserve(size);
    std::copy(addr, addr + size, std::back_inserter(data));
    std::reverse(data.begin(), data.end());
    addr += size;

    return {addr, std::move(data)};
}

auto Decoder::decode_timeout(uint8_t const* addr) const
    -> std::pair<uint8_t const*, timeout_type>
{
    ++addr;  // FIXME skip operand type, assume it's correct (OT_INT)

    auto t = timeout_type{0};
    {
        using viua::util::memory::aligned_read;
        aligned_read(t) = addr;
        addr += sizeof(t);
    }

    return {addr, be32toh(t)};
}

auto Decoder::decode_f64(uint8_t const* addr) const
    -> std::pair<uint8_t const*, double>
{
    auto v = double{0};
    {
        using viua::util::memory::aligned_read;
        aligned_read(v) = addr;
        addr += sizeof(v);
    }

    return {addr, v};
}

auto Decoder::decode_i32(uint8_t const* addr) const
    -> std::pair<uint8_t const*, int32_t>
{
    ++addr;  // FIXME skip operand type, assume it's correct (OT_INT)

    auto v = int32_t{0};
    {
        using viua::util::memory::aligned_read;
        aligned_read(v) = addr;
        addr += sizeof(v);
    }

    return {addr, static_cast<int32_t>(be32toh(static_cast<uint32_t>(v)))};
}

auto Decoder::decode_bool(uint8_t const* addr) const
    -> std::pair<uint8_t const*, bool>
{
    auto v = (static_cast<OperandType>(*addr) == OT_TRUE);
    ++addr;
    return {addr, v};
}

auto Decoder::decode_address(uint8_t const* addr) const
    -> std::pair<uint8_t const*, uint64_t>
{
    auto v = uint64_t{0};
    {
        using viua::util::memory::aligned_read;
        aligned_read(v) = addr;
        addr += sizeof(v);
    }

    return {addr, be64toh(v)};
}
}}}}  // namespace viua::bytecode::codec::main
