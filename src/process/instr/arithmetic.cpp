/*
 *  Copyright (C) 2016 Marek Marecki
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

#include <memory>
#include <functional>
#include <viua/bytecode/bytetypedef.h>
#include <viua/bytecode/decoder/operands.h>
#include <viua/types/type.h>
#include <viua/types/number.h>
#include <viua/types/integer.h>
#include <viua/types/float.h>
#include <viua/types/boolean.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/assert.h>
using namespace std;


byte* viua::process::Process::opadd(byte* addr) {
    OperandType result_type = OperandType::OT_VOID;
    tie(addr, result_type) = viua::bytecode::decoder::operands::fetch_operand_type(addr);

    unsigned target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, this);

    viua::types::Type* lhs_raw = nullptr;
    tie(addr, lhs_raw) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    auto lhs = static_cast<viua::types::numeric::Number*>(lhs_raw);

    viua::types::Type* rhs_raw = nullptr;
    tie(addr, rhs_raw) = viua::bytecode::decoder::operands::fetch_object(addr, this);
    auto rhs = static_cast<viua::types::numeric::Number*>(rhs_raw);

    if (result_type == OperandType::OT_INT
        or result_type == OperandType::OT_INT8
        or result_type == OperandType::OT_INT16
        or result_type == OperandType::OT_INT32
        or result_type == OperandType::OT_INT64
    ) {
        place(target, unique_ptr<viua::types::Type>{new viua::types::Integer(lhs->as_int32() + rhs->as_int32())});
    } else if (result_type == OperandType::OT_UINT
        or result_type == OperandType::OT_UINT8
        or result_type == OperandType::OT_UINT16
        or result_type == OperandType::OT_UINT32
        or result_type == OperandType::OT_UINT64
    ) {
        place(target, unique_ptr<viua::types::Type>{new viua::types::Integer(lhs->as_int32() + rhs->as_int32())});
    } else if (result_type == OperandType::OT_FLOAT or result_type == OperandType::OT_FLOAT32 or result_type == OperandType::OT_FLOAT64) {
        place(target, unique_ptr<viua::types::Type>{new viua::types::Float(lhs->as_float64() + rhs->as_float64())});
    }

    return addr;
}
