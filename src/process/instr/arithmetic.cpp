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
#include <viua/types/exception.h>
#include <viua/exceptions.h>
#include <viua/kernel/kernel.h>
#include <viua/assert.h>
using namespace std;

template<template <typename T> class Operator> static void perform_arithmetic(OperandType result_type, viua::process::Process* process, unsigned target, viua::types::numeric::Number* lhs, viua::types::numeric::Number* rhs) {
    if (result_type == OperandType::OT_INT) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Integer(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_INT8) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Integer(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_INT16) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Integer(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_INT32) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Integer(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_INT64) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Integer(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_UINT) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Integer(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_UINT8) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Integer(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_UINT16) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Integer(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_UINT32) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Integer(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_UINT64) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Integer(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_FLOAT) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Float(Operator<double>()(lhs->as_float64(), rhs->as_float64()))});
    } else if (result_type == OperandType::OT_FLOAT32) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Float(Operator<double>()(lhs->as_float64(), rhs->as_float64()))});
    } else if (result_type == OperandType::OT_FLOAT64) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Float(Operator<double>()(lhs->as_float64(), rhs->as_float64()))});
    } else {
        throw new viua::types::Exception("invalid operand type: illegal result type");
    }
}

template<template <typename T> class Operator> static byte* decode_operands_and_perform_arithmetic(byte* addr, viua::process::Process *process) {
    OperandType result_type = OperandType::OT_VOID;
    tie(addr, result_type) = viua::bytecode::decoder::operands::fetch_operand_type(addr);

    unsigned target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, process);

    viua::types::Type* lhs_raw = nullptr;
    tie(addr, lhs_raw) = viua::bytecode::decoder::operands::fetch_object(addr, process);

    viua::types::Type* rhs_raw = nullptr;
    tie(addr, rhs_raw) = viua::bytecode::decoder::operands::fetch_object(addr, process);

    using viua::types::numeric::Number;

    viua::assertions::expect_types<Number>("Number", lhs_raw, rhs_raw);


    auto lhs = static_cast<viua::types::numeric::Number*>(lhs_raw);
    auto rhs = static_cast<viua::types::numeric::Number*>(rhs_raw);

    perform_arithmetic<Operator>(result_type, process, target, lhs, rhs);

    return addr;
}

template<template <typename T> class Operator> static void perform_comparison(OperandType result_type, viua::process::Process* process, unsigned target, viua::types::numeric::Number* lhs, viua::types::numeric::Number* rhs) {
    if (result_type == OperandType::OT_INT) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Boolean(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_INT8) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Boolean(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_INT16) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Boolean(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_INT32) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Boolean(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_INT64) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Boolean(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_UINT) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Boolean(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_UINT8) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Boolean(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_UINT16) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Boolean(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_UINT32) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Boolean(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_UINT64) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Boolean(Operator<int32_t>()(lhs->as_int32(), rhs->as_int32()))});
    } else if (result_type == OperandType::OT_FLOAT) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Boolean(Operator<double>()(lhs->as_float64(), rhs->as_float64()))});
    } else if (result_type == OperandType::OT_FLOAT32) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Boolean(Operator<double>()(lhs->as_float64(), rhs->as_float64()))});
    } else if (result_type == OperandType::OT_FLOAT64) {
        process->put(target, unique_ptr<viua::types::Type>{new viua::types::Boolean(Operator<double>()(lhs->as_float64(), rhs->as_float64()))});
    } else {
        throw new viua::types::Exception("invalid operand type: illegal result type");
    }
}

template<template <typename T> class Operator> static byte* decode_operands_and_perform_comparison(byte* addr, viua::process::Process *process) {
    OperandType result_type = OperandType::OT_VOID;
    tie(addr, result_type) = viua::bytecode::decoder::operands::fetch_operand_type(addr);

    unsigned target = 0;
    tie(addr, target) = viua::bytecode::decoder::operands::fetch_register_index(addr, process);

    viua::types::Type* lhs_raw = nullptr;
    tie(addr, lhs_raw) = viua::bytecode::decoder::operands::fetch_object(addr, process);

    viua::types::Type* rhs_raw = nullptr;
    tie(addr, rhs_raw) = viua::bytecode::decoder::operands::fetch_object(addr, process);

    using viua::types::numeric::Number;

    viua::assertions::expect_types<Number>("Number", lhs_raw, rhs_raw);

    auto lhs = static_cast<viua::types::numeric::Number*>(lhs_raw);
    auto rhs = static_cast<viua::types::numeric::Number*>(rhs_raw);

    perform_comparison<Operator>(result_type, process, target, lhs, rhs);

    return addr;
}

byte* viua::process::Process::opadd(byte* addr) {
    return decode_operands_and_perform_arithmetic<plus>(addr, this);
}

byte* viua::process::Process::opsub(byte* addr) {
    return decode_operands_and_perform_arithmetic<minus>(addr, this);
}

byte* viua::process::Process::opmul(byte* addr) {
    return decode_operands_and_perform_arithmetic<multiplies>(addr, this);
}

byte* viua::process::Process::opdiv(byte* addr) {
    return decode_operands_and_perform_arithmetic<divides>(addr, this);
}

byte* viua::process::Process::oplt(byte* addr) {
    return decode_operands_and_perform_comparison<less>(addr, this);
}

byte* viua::process::Process::oplte(byte* addr) {
    return decode_operands_and_perform_comparison<less_equal>(addr, this);
}

byte* viua::process::Process::opgt(byte* addr) {
    return decode_operands_and_perform_comparison<greater>(addr, this);
}

byte* viua::process::Process::opgte(byte* addr) {
    return decode_operands_and_perform_comparison<greater_equal>(addr, this);
}

byte* viua::process::Process::opeq(byte* addr) {
    return decode_operands_and_perform_comparison<equal_to>(addr, this);
}
