/*
 *  Copyright (C) 2021-2025 Marek Marecki
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
#include <string.h>

#include <format>
#include <iostream>

#include <viua/arch/arch.h>
#include <viua/support/binarith.hh>
#include <viua/support/fdstream.h>
#include <viua/vm/backtrace.h>
#include <viua/vm/ins.h>


namespace viua {
extern viua::support::fdstream TRACE_STREAM;
}

namespace viua::vm::ins {
using namespace viua::arch::ins;
using viua::vm::Stack;

auto dump_globals(
    Stack const& stack) -> void
{
    viua::TRACE_STREAM << "  globals:" << viua::TRACE_STREAM.endl;

    auto const& atoms   = stack.proc->atoms;
    auto const& globals = stack.proc->globals;

    for (auto const& [key, each] : globals) {
        viua::TRACE_STREAM << "    " << atoms.at(key) << " = ";

        if (each.is_void()) {
            /* do nothing */
        } else if (auto const v = each.get<register_type::undefined_type>();
                   v) {
            TRACE_STREAM << "raw" << std::hex << std::setfill('0');
            for (auto const each : *v) {
                TRACE_STREAM << " " << std::setw(2)
                             << static_cast<unsigned>(each);
            }
            TRACE_STREAM << '\n';
        } else if (auto const v = each.get<int64_t>(); v) {
            TRACE_STREAM << "is " << std::hex << std::setw(16)
                         << std::setfill('0') << *v << " " << std::dec << *v
                         << '\n';
        } else if (auto const v = each.get<uint64_t>(); v) {
            TRACE_STREAM << "iu " << std::hex << std::setw(16)
                         << std::setfill('0') << *v << " " << std::dec << *v
                         << '\n';
        } else if (auto const v = each.get<float>(); v) {
            auto const precision = std::cerr.precision();
            TRACE_STREAM
                << "fl " << std::hexfloat << *v << " " << std::defaultfloat
                << std::setprecision(std::numeric_limits<float>::digits10 + 1)
                << *v << '\n';
            TRACE_STREAM << std::setprecision(precision);
        } else if (auto const v = each.get<double>(); v) {
            auto const precision = std::cerr.precision();
            TRACE_STREAM
                << "db " << std::hexfloat << *v << " " << std::defaultfloat
                << std::setprecision(std::numeric_limits<double>::digits10 + 1)
                << *v << '\n';
            TRACE_STREAM << std::setprecision(precision);
        } else if (auto const v = each.get<register_type::pointer_type>(); v) {
            TRACE_STREAM << "ptr " << std::hex << std::setw(16)
                         << std::setfill('0') << v->ptr << " " << std::dec
                         << v->ptr << '\n';
        } else if (auto const v = each.get<register_type::atom_type>(); v) {
            TRACE_STREAM << "atom " << atoms.at(v->key) << '\n';
        } else if (auto const v = each.get<register_type::pid_type>(); v) {
            TRACE_STREAM << "pid " << viua::runtime::PID{ *v }.to_string()
                         << '\n';
        }
    }
}
auto execute(
    EBREAK const,
    Stack& stack,
    ip_type const) -> void
{
    viua::TRACE_STREAM << "begin ebreak in process "
                       << stack.proc->pid.to_string()
                       << viua::TRACE_STREAM.endl;

    viua::TRACE_STREAM << "  backtrace:" << viua::TRACE_STREAM.endl;
    viua::vm::backtrace::print_backtrace(stack);

    viua::TRACE_STREAM << "  register contents:" << viua::TRACE_STREAM.endl;
    for (auto i = size_t{ 0 }; i < stack.frames.size(); ++i) {
        auto const& each = stack.frames.at(i);

        viua::TRACE_STREAM << "    of #" << i << viua::TRACE_STREAM.endl;

        auto const fptr = each.saved.fp;
        auto const sbrk = each.saved.sbrk;
        TRACE_STREAM << "        [fptr] "
                     << "iu " << std::hex << std::setw(16) << std::setfill('0')
                     << fptr << " " << std::dec << fptr << '\n';
        TRACE_STREAM << "        [sbrk] "
                     << "iu " << std::hex << std::setw(16) << std::setfill('0')
                     << sbrk << " " << std::dec << sbrk << '\n';

        viua::vm::backtrace::dump_registers(
            each.parameters, stack.proc->atoms, "p");
        viua::vm::backtrace::dump_registers(
            each.registers, stack.proc->atoms, "l");
    }
    viua::vm::backtrace::dump_registers(stack.args, stack.proc->atoms, "a");

    viua::vm::backtrace::dump_memory(stack.proc->memory);

    dump_globals(stack);

    viua::TRACE_STREAM << "  environment:" << viua::TRACE_STREAM.endl;
    {
        auto const aw = stack.proc->arithmetic_width;

        using namespace viua::arithmetic;
        auto const smax = static_cast<int64_t>(signed_type::max(aw));
        auto const smin = static_cast<int64_t>(signed_type::min(aw));
        auto const umax = static_cast<uint64_t>(unsigned_type::max(aw));

        TRACE_STREAM << std::format("    [earw] 0x{:02x} {:<2d}  [{}, {}] {}u",
                                    aw,
                                    aw,
                                    smin,
                                    smax,
                                    umax)
                     << TRACE_STREAM.endl;
    }

    viua::TRACE_STREAM << "end ebreak in process "
                       << stack.proc->pid.to_string()
                       << viua::TRACE_STREAM.endl;
}
}  // namespace viua::vm::ins
