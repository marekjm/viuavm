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

#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/arch/ins.h>
#include <viua/arch/ops.h>
#include <viua/libexec/common.hh>
#include <viua/support/errno.h>
#include <viua/support/fdstream.h>
#include <viua/support/print.hh>
#include <viua/support/string.h>
#include <viua/support/tty.h>
#include <viua/vm/backtrace.h>
#include <viua/vm/core.h>
#include <viua/vm/elf.h>
#include <viua/vm/ins.h>

#if defined(VIUAVM_IO_IMPL_CLASSIC)
#include <viua/vm/io/impl/classic.hh>
#elif defined(VIUAVM_IO_IMPL_IO_URING)
#include <viua/vm/io/impl/io_uring.hh>
#else
#error "no I/O implementation selected"
#endif


constexpr auto VIUA_SLOW_CYCLES = false;

namespace viua {
auto TRACE_STREAM = viua::support::fdstream{ 2 };
}

namespace {
auto format_time(
    std::chrono::microseconds const us) -> std::string
{
    auto out = std::ostringstream{};
    out << std::fixed << std::setprecision(2);

    auto const c = static_cast<double>(us.count());
    if (c > 1e6) {
        out << (c / 1.0e6) << "s";
    } else if (c > 1e3) {
        out << (c / 1.0e3) << "ms";
    } else {
        out << c << "us";
    }
    return out.str();
}
auto format_hz(
    double const hz) -> std::string
{
    auto out = std::ostringstream{};
    if (hz > 1e3) {
        out << std::fixed << std::setprecision(2);
        out << (hz / 1.0e3) << " kHz";
    } else {
        out << hz << " Hz";
    }
    return out.str();
}
auto run(
    viua::vm::Process& proc) -> bool
{
    auto const ip_ok = [&proc]() -> bool
    { return proc.module.ip_in_valid_range(proc.stack.ip); };

    if constexpr (viua::vm::ins::VIUA_TRACE_CYCLES) {
        viua::TRACE_STREAM
            << "cycle at " << proc.module.elf_path.native() << "[.text+0x"
            << std::hex << std::setw(16) << std::setfill('0')
            << ((proc.stack.ip - proc.module.ip_base)
                * sizeof(viua::arch::instruction_type))
            << std::dec << "] in process " << proc.pid.to_string()
            << viua::TRACE_STREAM.endl;
    }

    using viua::vm::PREEMPTION_THRESHOLD;
    for (auto i = size_t{ 0 }; i < PREEMPTION_THRESHOLD and ip_ok(); ++i) {
        proc.stack.ip = viua::vm::ins::execute(proc.stack, proc.stack.ip);
        ++proc.core->perf_counters.total_ops_executed;
    }

    if (proc.stack.frames.empty()) {
        viua::TRACE_STREAM << "[vm:sched:proc] process " << proc.pid.to_string()
                           << " has empty stack" << viua::TRACE_STREAM.endl;
        return false;
    }
    if (not ip_ok()) {
        auto const bad_address = ((proc.stack.ip - proc.module.ip_base)
                                  * sizeof(viua::arch::instruction_type));
        std::println(
            stderr, "[vm] ip {:08x} outside of valid range", bad_address);
        return false;
    }

    if constexpr (VIUA_SLOW_CYCLES) {
        /*
         * FIXME Limit the amount of instructions executed per second
         * for debugging purposes. Once everything works as it should,
         * remove this code.
         */
        using namespace std::literals;
        std::this_thread::sleep_for(160ms);
    }

    return true;
}
auto run(
    viua::vm::Core& core) -> void
{
    core.perf_counters.start();

    while (not core.run_queue.empty()) {
        auto proc = core.pop_ready();

        auto const state = run(*proc);

        if (state) {
            core.push_ready(std::move(proc));
        } else {
            viua::TRACE_STREAM << "[vm:sched:proc] process "
                               << proc->pid.to_string() << " exited"
                               << viua::TRACE_STREAM.endl;
        }
    }

    core.perf_counters.stop();
    {
        auto const total_ops = core.perf_counters.total_ops_executed;
        auto const total_us =
            std::chrono::duration_cast<std::chrono::microseconds>(
                core.perf_counters.duration());
        auto const approx_hz = (1e6 / static_cast<double>(total_us.count()))
                               * static_cast<double>(total_ops);
        viua::TRACE_STREAM << std::setfill(' ') << std::dec;
        viua::TRACE_STREAM << "[vm:perf] executed ops " << total_ops
                           << ", run time " << format_time(total_us)
                           << viua::TRACE_STREAM.endl;
        viua::TRACE_STREAM << "[vm:perf] approximate frequency "
                           << format_hz(approx_hz) << viua::TRACE_STREAM.endl;
    }
}
}  // namespace

auto main(
    int argc,
    char* argv[]) -> int
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    using viua::libexec::Args;
    auto const args = viua::libexec::args_or_exit("vm",
                                                  argc,
                                                  argv,
                                                  {
                                                      VIUA_TOOL_COMMON_OPTIONS,
                                                  });
    if (args.args.empty()) {
        std::println(stderr,
                     "{}error{}: no executable to run",
                     esc(2, COLOR_FG_RED),
                     esc(2, ATTR_RESET));
        return 1;
    }

    /*
     * Do not assume that the path given by the user points to a file that
     * exists. Typos are a thing. And let's check if the file really is a
     * regular file - trying to execute directories or device files does not
     * make much sense.
     */
    auto const elf_path = std::filesystem::path{ args.args.front() };
    if (not std::filesystem::exists(elf_path)) {
        viua::support::errorln("file does not exist: {}{}{}",
                               esc(2, COLOR_FG_WHITE),
                               elf_path.native(),
                               esc(2, ATTR_RESET));
        return 1;
    }
    {
        struct stat statbuf{};
        if (stat(elf_path.c_str(), &statbuf) == -1) {
            auto const saved_errno = errno;
            auto const errname     = viua::support::errno_name(saved_errno);
            auto const errdesc     = viua::support::errno_desc(saved_errno);

            viua::support::errorln(elf_path, "{}: {}", errname, errdesc);
            return 1;
        }
        if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
            viua::support::errorln(elf_path, "not a regular file");
            return 1;
        }
    }

    /*
     * Even if the path exists and is a regular file we should check if it was
     * opened correctly.
     */
    auto const elf_fd = open(elf_path.c_str(), O_RDONLY);
    if (elf_fd == -1) {
        auto const saved_errno = errno;
        auto const errname     = viua::support::errno_name(saved_errno);
        auto const errdesc     = viua::support::errno_desc(saved_errno);

        viua::support::errorln(elf_path, "{}: {}", errname, errdesc);
        return 1;
    }

    using Module           = viua::vm::elf::Loaded_elf;
    auto const main_module = Module::load(elf_fd);

    if (main_module.header.e_type != ET_EXEC) {
        viua::support::errorln(elf_path, "not an executable file");
        return 1;
    }

    if (auto const f = main_module.find_fragment(".rodata");
        not f.has_value()) {
        viua::support::errorln(elf_path, "no strings fragment found");
        viua::support::noteln(elf_path, "no .rodata section found");
        return 1;
    }
    if (auto const f = main_module.find_fragment(".symtab");
        not f.has_value()) {
        viua::support::errorln(elf_path, "no function table fragment found");
        viua::support::noteln(elf_path, "no .symtab section found");
        return 1;
    }
    if (auto const f = main_module.find_fragment(".strtab");
        not f.has_value()) {
        viua::support::errorln(elf_path, "no string table fragment found");
        viua::support::noteln(elf_path, "no .strtab section found");
        return 1;
    }
    if (auto const f = main_module.find_fragment(".text"); not f.has_value()) {
        viua::support::errorln(elf_path, "no text fragment found");
        viua::support::noteln(elf_path, "no .text section found");
        return 1;
    }

    auto entry_addr = size_t{ 0 };
    if (auto const ep = main_module.entry_point(); ep.has_value()) {
        entry_addr = (*ep / sizeof(viua::arch::instruction_type));
    } else {
        viua::support::errorln(elf_path, "no entry point defined");
        return 1;
    }

    auto io = viua::vm::io::impl::VIUAVM_IO_IMPL::IO{};
    {
        /*
         * Watch all standard streams (input, output, and error). Any function
         * which creates a file descriptor MUST register it in the I/O
         * scheduler.
         */
        io.watch(0);
        io.watch(1);
        io.watch(2);
    }
    auto core = viua::vm::Core{ io };
    core.modules.emplace("", viua::vm::Module{ elf_path, main_module });
    auto const main_pid [[maybe_unused]] = core.spawn("", entry_addr);

    if constexpr (viua::vm::ins::VIUA_TRACE_CYCLES) {
        if (auto trace_fd = getenv("VIUA_VM_TRACE_FD"); trace_fd) {
            try {
                /*
                 * Assume an file descriptor opened for writing was received.
                 */
                viua::TRACE_STREAM =
                    viua::support::fdstream{ std::stoi(trace_fd) };
            } catch (std::invalid_argument const&) {
                /*
                 * Otherwise, treat the thing received as a filename and open it
                 * for writing.
                 */
                viua::TRACE_STREAM = viua::support::fdstream{ open(
                    trace_fd, O_WRONLY | O_CLOEXEC) };
            }
        }
    }

    try {
        run(core);
    } catch (viua::vm::abort_execution const& e) {
        auto const aborted_ip = ((e.stack.ip - e.stack.proc->module.ip_base)
                                 * sizeof(viua::arch::instruction_type));

        std::println(stderr, "Aborted: {}", e.what());
        std::println(stderr, "Aborted IP: 0x{:016x}", aborted_ip);
        std::println(stderr, "Aborted instruction: 0x{:016x}", *e.stack.ip);
        viua::vm::backtrace::print_backtrace(e.stack);

        auto const& stack = e.stack;

        for (auto i = size_t{ 0 }; i < stack.frames.size(); ++i) {
            auto const& each = stack.frames.at(i);

            std::println(stderr, "registers of #{}", i);

            /*
            auto const fptr = each.saved.fp;
            auto const sbrk = each.saved.sbrk;
            TRACE_STREAM << "        [fptr] "
                         << "iu " << std::hex << std::setw(16) <<
            std::setfill('0')
                         << fptr << " " << std::dec << fptr << '\n';
            TRACE_STREAM << "        [sbrk] "
                         << "iu " << std::hex << std::setw(16) <<
            std::setfill('0')
                         << sbrk << " " << std::dec << sbrk << '\n';
                         */

            viua::vm::backtrace::dump_registers(
                each.parameters, stack.proc->atoms, "p");
            viua::vm::backtrace::dump_registers(
                each.registers, stack.proc->atoms, "l");
        }

        if constexpr (true) {
            throw;
        } else {
            return 1;
        }
    }

    return 0;
}
