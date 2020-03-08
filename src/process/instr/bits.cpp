/*
 *  Copyright (C) 2017-2020 Marek Marecki
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

#include <iostream>
#include <memory>
#include <viua/bytecode/bytetypedef.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/process.h>
#include <viua/types/bits.h>
#include <viua/types/boolean.h>
#include <viua/types/integer.h>
#include <viua/util/memory.h>

using viua::internals::types::Op_address_type;
using viua::util::memory::load_aligned;


auto viua::process::Process::opbits_of_integer(Op_address_type addr)
    -> Op_address_type
{
    auto target  = decoder.fetch_register(addr, *this);
    auto const n = decoder.fetch_value_of<viua::types::Integer>(addr, *this);

    auto const size_in_bits = sizeof(viua::types::Integer::underlying_type) * 8;
    auto decomposed         = std::vector<bool>(size_in_bits);
    auto const base_mask    = viua::types::Integer::underlying_type{1};
    for (auto i = decltype(size_in_bits){0}; i < size_in_bits; ++i) {
        decomposed.at(i) =
            (n->as_unsigned() & static_cast<uint64_t>(base_mask << i));
    }

    *target = std::make_unique<viua::types::Bits>(std::move(decomposed));

    return addr;
}

auto viua::process::Process::opinteger_of_bits(Op_address_type addr)
    -> Op_address_type
{
    auto target  = decoder.fetch_register(addr, *this);
    auto const b = decoder.fetch_value_of<viua::types::Bits>(addr, *this);

    auto const size_in_bits = b->size();
    auto decomposed         = std::vector<bool>(size_in_bits);
    auto const base_mask    = viua::types::Integer::underlying_type{1};
    auto n                  = viua::types::Integer::underlying_type{0};
    for (auto i = decltype(size_in_bits){0}; i < size_in_bits; ++i) {
        if (b->at(i)) {
            n = (n | (base_mask << i));
        }
    }

    *target = std::make_unique<viua::types::Integer>(n);

    return addr;
}


auto viua::process::Process::opbits(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_register(addr, *this);

    auto ot = viua::bytecode::codec::main::get_operand_type(addr);
    if (ot == OT_BITS) {
        auto data = decoder.fetch_bits_string(addr);
        *target   = std::make_unique<viua::types::Bits>(std::move(data));
    } else {
        auto n  = decoder.fetch_value_of<viua::types::Integer>(addr, *this);
        *target = std::make_unique<viua::types::Bits>(n->as_unsigned());
    }

    return addr;
}


auto viua::process::Process::opbitand(Op_address_type addr) -> Op_address_type
{
    auto target    = decoder.fetch_register(addr, *this);
    auto const lhs = decoder.fetch_value_of<viua::types::Bits>(addr, *this);
    auto const rhs = decoder.fetch_value_of<viua::types::Bits>(addr, *this);

    *target = (*lhs) & (*rhs);

    return addr;
}


auto viua::process::Process::opbitor(Op_address_type addr) -> Op_address_type
{
    auto target    = decoder.fetch_register(addr, *this);
    auto const lhs = decoder.fetch_value_of<viua::types::Bits>(addr, *this);
    auto const rhs = decoder.fetch_value_of<viua::types::Bits>(addr, *this);

    *target = (*lhs) | (*rhs);

    return addr;
}


auto viua::process::Process::opbitnot(Op_address_type addr) -> Op_address_type
{
    auto target       = decoder.fetch_register(addr, *this);
    auto const source = decoder.fetch_value_of<viua::types::Bits>(addr, *this);

    *target = source->inverted();

    return addr;
}


auto viua::process::Process::opbitxor(Op_address_type addr) -> Op_address_type
{
    auto target    = decoder.fetch_register(addr, *this);
    auto const lhs = decoder.fetch_value_of<viua::types::Bits>(addr, *this);
    auto const rhs = decoder.fetch_value_of<viua::types::Bits>(addr, *this);

    *target = (*lhs) ^ (*rhs);

    return addr;
}


auto viua::process::Process::opbitat(Op_address_type addr) -> Op_address_type
{
    auto target     = decoder.fetch_register(addr, *this);
    auto const bits = decoder.fetch_value_of<viua::types::Bits>(addr, *this);
    auto const n    = decoder.fetch_value_of<viua::types::Integer>(addr, *this);

    *target =
        std::make_unique<viua::types::Boolean>(bits->at(n->as_unsigned()));

    return addr;
}


auto viua::process::Process::opbitset(Op_address_type addr) -> Op_address_type
{
    auto target = decoder.fetch_value_of<viua::types::Bits>(addr, *this);
    auto const index =
        decoder.fetch_value_of<viua::types::Integer>(addr, *this);

    bool value = false;
    auto ot    = viua::bytecode::codec::main::get_operand_type(addr);
    if (ot & OperandType::OT_BOOL) {
        value = decoder.fetch_bool(addr);
    } else {
        auto const x =
            decoder.fetch_value_of<viua::types::Boolean>(addr, *this);
        value = x->boolean();
    }

    target->set(index->as_unsigned(), value);

    return addr;
}

template<typename T>
using dumb_ptr = T*;  // FIXME; use std::experimental::observer_ptr

using BitShiftOp = decltype(&viua::types::Bits::shl);
template<const BitShiftOp op>
static auto execute_bit_shift_instruction(viua::process::Process* process,
                                          Op_address_type addr)
    -> Op_address_type
{
    auto target = process->decoder.fetch_register_or_void(addr, *process);

    auto const source =
        process->decoder.fetch_value_of<viua::types::Bits>(addr, *process);
    auto const offset =
        process->decoder.fetch_value_of<viua::types::Integer>(addr, *process);

    /*
     * Let's hope the compiler sees that the 'op' can be resolved at compile
     * time, and optimise the extra pointer dereference away.
     */
    auto result = (source->*op)(offset->as_unsigned());
    if (target.has_value()) {
        **target = std::move(result);
    }

    return addr;
}

auto viua::process::Process::opshl(Op_address_type addr) -> Op_address_type
{
    return execute_bit_shift_instruction<&viua::types::Bits::shl>(this, addr);
}


auto viua::process::Process::opshr(Op_address_type addr) -> Op_address_type
{
    return execute_bit_shift_instruction<&viua::types::Bits::shr>(this, addr);
}


auto viua::process::Process::opashl(Op_address_type addr) -> Op_address_type
{
    return execute_bit_shift_instruction<&viua::types::Bits::ashl>(this, addr);
}


auto viua::process::Process::opashr(Op_address_type addr) -> Op_address_type
{
    return execute_bit_shift_instruction<&viua::types::Bits::ashr>(this, addr);
}

using BitRotateOp = decltype(&viua::types::Bits::rol);
template<BitRotateOp const op>
static auto execute_bit_rotate_op(viua::process::Process* process,
                                  Op_address_type addr) -> Op_address_type
{
    auto target =
        process->decoder.fetch_value_of<viua::types::Bits>(addr, *process);
    auto const offset =
        process->decoder.fetch_value_of<viua::types::Integer>(addr, *process);

    (target->*op)(offset->as_unsigned());

    return addr;
}

auto viua::process::Process::oprol(Op_address_type addr) -> Op_address_type
{
    return execute_bit_rotate_op<&viua::types::Bits::rol>(this, addr);
}


auto viua::process::Process::opror(Op_address_type addr) -> Op_address_type
{
    return execute_bit_rotate_op<&viua::types::Bits::ror>(this, addr);
}

using BitsIncrementDecrementOp = decltype(&viua::types::Bits::increment);
template<BitsIncrementDecrementOp const op>
static auto execute_increment_decrement_op(viua::process::Process* process,
                                           Op_address_type addr)
    -> Op_address_type
{
    auto target =
        process->decoder.fetch_value_of<viua::types::Bits>(addr, *process);

    (target->*op)();

    return addr;
}

using BitsArithmeticOp = decltype(&viua::types::Bits::wrapadd);
template<BitsArithmeticOp const op>
static auto execute_arithmetic_op(viua::process::Process* process,
                                  Op_address_type addr) -> Op_address_type
{
    auto target = process->decoder.fetch_register(addr, *process);
    auto const lhs =
        process->decoder.fetch_value_of<viua::types::Bits>(addr, *process);
    auto const rhs =
        process->decoder.fetch_value_of<viua::types::Bits>(addr, *process);

    *target = (lhs->*op)(*rhs);

    return addr;
}

auto viua::process::Process::opwrapincrement(Op_address_type addr)
    -> Op_address_type
{
    return execute_increment_decrement_op<&viua::types::Bits::increment>(this,
                                                                         addr);
}
auto viua::process::Process::opwrapdecrement(Op_address_type addr)
    -> Op_address_type
{
    return execute_increment_decrement_op<&viua::types::Bits::decrement>(this,
                                                                         addr);
}
auto viua::process::Process::opwrapadd(Op_address_type addr) -> Op_address_type
{
    return execute_arithmetic_op<&viua::types::Bits::wrapadd>(this, addr);
}
auto viua::process::Process::opwrapsub(Op_address_type addr) -> Op_address_type
{
    return execute_arithmetic_op<&viua::types::Bits::wrapsub>(this, addr);
}
auto viua::process::Process::opwrapmul(Op_address_type addr) -> Op_address_type
{
    return execute_arithmetic_op<&viua::types::Bits::wrapmul>(this, addr);
}
auto viua::process::Process::opwrapdiv(Op_address_type addr) -> Op_address_type
{
    return execute_arithmetic_op<&viua::types::Bits::wrapdiv>(this, addr);
}


auto viua::process::Process::opcheckedsincrement(Op_address_type addr)
    -> Op_address_type
{
    return execute_increment_decrement_op<
        &viua::types::Bits::checked_signed_increment>(this, addr);
}
auto viua::process::Process::opcheckedsdecrement(Op_address_type addr)
    -> Op_address_type
{
    return execute_increment_decrement_op<
        &viua::types::Bits::checked_signed_decrement>(this, addr);
}
auto viua::process::Process::opcheckedsadd(Op_address_type addr)
    -> Op_address_type
{
    return execute_arithmetic_op<&viua::types::Bits::checked_signed_add>(this,
                                                                         addr);
}
auto viua::process::Process::opcheckedssub(Op_address_type addr)
    -> Op_address_type
{
    return execute_arithmetic_op<&viua::types::Bits::checked_signed_sub>(this,
                                                                         addr);
}
auto viua::process::Process::opcheckedsmul(Op_address_type addr)
    -> Op_address_type
{
    return execute_arithmetic_op<&viua::types::Bits::checked_signed_mul>(this,
                                                                         addr);
}
auto viua::process::Process::opcheckedsdiv(Op_address_type addr)
    -> Op_address_type
{
    return execute_arithmetic_op<&viua::types::Bits::checked_signed_div>(this,
                                                                         addr);
}


auto viua::process::Process::opsaturatingsincrement(Op_address_type addr)
    -> Op_address_type
{
    return execute_increment_decrement_op<
        &viua::types::Bits::saturating_signed_increment>(this, addr);
}
auto viua::process::Process::opsaturatingsdecrement(Op_address_type addr)
    -> Op_address_type
{
    return execute_increment_decrement_op<
        &viua::types::Bits::saturating_signed_decrement>(this, addr);
}
auto viua::process::Process::opsaturatingsadd(Op_address_type addr)
    -> Op_address_type
{
    return execute_arithmetic_op<&viua::types::Bits::saturating_signed_add>(
        this, addr);
}
auto viua::process::Process::opsaturatingssub(Op_address_type addr)
    -> Op_address_type
{
    return execute_arithmetic_op<&viua::types::Bits::saturating_signed_sub>(
        this, addr);
}
auto viua::process::Process::opsaturatingsmul(Op_address_type addr)
    -> Op_address_type
{
    return execute_arithmetic_op<&viua::types::Bits::saturating_signed_mul>(
        this, addr);
}
auto viua::process::Process::opsaturatingsdiv(Op_address_type addr)
    -> Op_address_type
{
    return execute_arithmetic_op<&viua::types::Bits::saturating_signed_div>(
        this, addr);
}
