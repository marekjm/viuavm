/*
 *  Copyright (C) 2025 Marek Marecki
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

#include <viua/support/fdstream.h>
#include <viua/vm/backtrace.h>


namespace viua {
extern viua::support::fdstream TRACE_STREAM;
}


namespace viua::vm::backtrace {
auto dump_registers(std::vector<viua::vm::Register> const& registers,
                    viua::vm::Process::atoms_map_type const& atoms,
                    std::string_view const suffix) -> void
{
    for (auto i = size_t{0}; i < registers.size(); ++i) {
        auto const& each = registers.at(i);
        if (each.is_void()) {
            continue;
        }

        TRACE_STREAM << "      " << std::setw(7) << std::setfill(' ')
                     << ('[' + std::to_string(i) + '.' + suffix.data() + ']')
                     << ' ';

        using register_type = viua::vm::Register;
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
            TRACE_STREAM << "pid " << viua::runtime::PID{*v}.to_string()
                         << '\n';
        }
    }
}
auto print_backtrace_line(viua::vm::Stack const& stack, size_t const frame_index) -> void
{
    auto const& elf  = stack.proc->module.elf;
    auto const& each = stack.frames.at(frame_index);

    auto entry_off =
        static_cast<size_t>(each.entry_address - stack.proc->module.ip_base)
        * sizeof(viua::arch::instruction_type);
    auto const sym =
        std::find_if(elf.symtab.begin(),
                     elf.symtab.end(),
                     [entry_off](auto const& each) -> bool {
                         return (each.st_value == entry_off)
                                and (ELF64_ST_TYPE(each.st_info) == STT_FUNC);
                     });

    viua::TRACE_STREAM << "    #" << frame_index << "  ";
    viua::TRACE_STREAM
        << ((sym == elf.symtab.end()) ? "??" : elf.str_at(sym->st_name));
    viua::TRACE_STREAM << (each.parameters.empty() ? " ()" : " (...)");

    auto ip_offset = size_t{};
    if (frame_index < (stack.frames.size() - 1)) {
        ip_offset = (stack.frames.at(frame_index + 1).return_address
                     - stack.proc->module.ip_base);
    } else {
        ip_offset = (stack.ip - stack.proc->module.ip_base);
    }
    viua::TRACE_STREAM << " at " << stack.proc->module.elf_path.native()
                       << "[.text+0x" << std::hex << std::setw(8)
                       << std::setfill('0');
    viua::TRACE_STREAM << (ip_offset * sizeof(viua::arch::instruction_type));
    viua::TRACE_STREAM << std::dec << ']';

    viua::TRACE_STREAM << " return to ";
    if (each.return_address) {
        viua::TRACE_STREAM
            << stack.proc->module.elf_path.native() << "[.text+0x" << std::hex
            << std::setw(8) << std::setfill('0')
            << ((each.return_address - stack.proc->module.ip_base)
                * sizeof(viua::arch::instruction_type))
            << std::dec << ']';
    } else {
        viua::TRACE_STREAM << "null";
    }

    viua::TRACE_STREAM << viua::TRACE_STREAM.endl;
}
auto print_backtrace(viua::vm::Stack const& stack, std::optional<size_t> const only_for)
    -> void
{
    if (only_for.has_value()) {
        print_backtrace_line(stack, *only_for);
    } else {
        for (auto i = size_t{0}; i < stack.frames.size(); ++i) {
            print_backtrace_line(stack, i);
        }
    }
}
auto dump_memory(std::vector<viua::vm::Page> const& memory) -> void
{
    viua::TRACE_STREAM << "  memory:" << viua::TRACE_STREAM.endl;

    viua::TRACE_STREAM << std::hex << std::setfill('0');
    for (auto line = size_t{0}; line < (memory.front().size() / MEM_LINE_SIZE);
         ++line) {
        viua::TRACE_STREAM << "    ";
        viua::TRACE_STREAM
            << std::setw(16)
            << (MEM_FIRST_STACK_BREAK - ((line + 1) * MEM_LINE_SIZE) + 1)
            << "--" << std::setw(2)
            << ((MEM_FIRST_STACK_BREAK - (line * MEM_LINE_SIZE))
                & 0x00000000000000ff)
            << "  ";

        auto const& page = memory.front();
        auto at          = [&page, line](size_t const n) -> uint8_t {
            return *((page.data() + page.size() - 1)
                     - (line * MEM_LINE_SIZE + n));
        };
        for (auto i = MEM_LINE_SIZE; i; --i) {
            viua::TRACE_STREAM << std::setw(2) << static_cast<int>(at(i - 1))
                               << ' ';
        }
        viua::TRACE_STREAM << "| ";
        for (auto i = MEM_LINE_SIZE; i; --i) {
            auto const c = at(i - 1);
            viua::TRACE_STREAM << (isprint(c) ? static_cast<char>(c) : '.');
        }
        viua::TRACE_STREAM << viua::TRACE_STREAM.endl;
    }
}
}
