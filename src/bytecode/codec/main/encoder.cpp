/*
 *  Copyright (C) 2020 Marek Marecki
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
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/codec/main.h>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/operand_types.h>
#include <viua/util/memory.h>

namespace viua { namespace bytecode { namespace codec { namespace main {
using viua::util::memory::aligned_write;

auto Encoder::encode_opcode(uint8_t* addr, OPCODE const op) const -> uint8_t*
{
    *addr = op;
    return (addr + sizeof(OPCODE));
}

auto Encoder::encode_register(uint8_t* addr, Register_access const ra) const
    -> uint8_t*
{
    auto const [set, index, access] = ra;

    {
        switch (access) {
        case Access_specifier::Direct:
            *addr = OT_REGISTER_INDEX;
            break;
        case Access_specifier::Register_indirect:
            *addr = OT_REGISTER_REFERENCE;
            break;
        case Access_specifier::Pointer_dereference:
            *addr = OT_POINTER;
            break;
        default:
            assert(false);
        }
        addr += sizeof(OperandType);
    }

    {
        using set_type = std::underlying_type_t<viua::internals::Register_sets>;
        switch (set) {
        case Register_set::Global:
            *addr =
                static_cast<set_type>(viua::internals::Register_sets::GLOBAL);
            break;
        case Register_set::Local:
            *addr =
                static_cast<set_type>(viua::internals::Register_sets::LOCAL);
            break;
        case Register_set::Static:
            *addr =
                static_cast<set_type>(viua::internals::Register_sets::STATIC);
            break;
        case Register_set::Arguments:
            *addr = static_cast<set_type>(
                viua::internals::Register_sets::ARGUMENTS);
            break;
        case Register_set::Parameters:
            *addr = static_cast<set_type>(
                viua::internals::Register_sets::PARAMETERS);
            break;
        case Register_set::Closure_local:
            *addr = static_cast<set_type>(
                viua::internals::Register_sets::CLOSURE_LOCAL);
            break;
        default:
            assert(0);
        }
        addr += sizeof(set_type);
    }

    {
        aligned_write(addr) = htobe16(index);
        addr += sizeof(index);
    }

    return addr;
}

auto Encoder::encode_void(uint8_t* addr) const -> uint8_t*
{
    *addr = OT_VOID;
    return (addr + sizeof(OT_VOID));
}

auto Encoder::encode_f64(uint8_t* addr, double const n) const -> uint8_t*
{
    aligned_write(addr) = n;
    addr += sizeof(double);

    return addr;
}

auto Encoder::encode_i32(uint8_t* addr, int32_t const n) const -> uint8_t*
{
    *addr = OT_INT;
    addr += sizeof(OT_INT);

    aligned_write(addr) = static_cast<int32_t>(htobe32(static_cast<uint32_t>(n)));
    addr += sizeof(int32_t);

    return addr;
}

auto Encoder::encode_bool(uint8_t* addr, bool const n) const -> uint8_t*
{
    *addr = (n ? OT_TRUE : OT_FALSE);
    addr += sizeof(OT_TRUE);

    return addr;
}

auto Encoder::encode_string(uint8_t* addr, std::string const s) const
    -> uint8_t*
{
    *addr = OT_STRING;
    ++addr;

    return encode_raw_string(addr, s);
}

auto Encoder::encode_raw_string(uint8_t* addr, std::string const s) const
    -> uint8_t*
{
    std::copy(s.begin(), s.end(), addr);
    addr += s.size();

    *addr = '\0';
    ++addr;

    return addr;
}

auto Encoder::encode_bits_string(uint8_t* addr,
                                 std::vector<uint8_t> const data) const
    -> uint8_t*
{
    *addr = OT_BITS;
    ++addr;

    aligned_write(addr) = data.size();
    addr += sizeof(viua::internals::types::bits_size);

    std::copy(data.rbegin(), data.rend(), addr);
    addr += data.size();

    return addr;
}

auto Encoder::encode_timeout(uint8_t* addr, timeout_type const value) const -> uint8_t*
{
    *addr = OT_INT;
    ++addr;

    aligned_write(addr) = htobe32(value);
    addr += sizeof(timeout_type);

    return addr;
}

auto Encoder::encode_address(uint8_t* addr, uint64_t const dest) const -> uint8_t*
{
    auto const bedest = htobe64(dest);
    aligned_write(addr) = bedest;
    addr += sizeof(uint64_t);
    return addr;
}
}}}}  // namespace viua::bytecode::codec::main
