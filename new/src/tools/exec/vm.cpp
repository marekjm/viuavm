/*
 *  Copyright (C) 2021-2022 Marek Marecki
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
#include <liburing.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
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
#include <viua/support/fdstream.h>
#include <viua/support/string.h>
#include <viua/support/tty.h>
#include <viua/vm/core.h>
#include <viua/vm/elf.h>
#include <viua/vm/ins.h>
#include <viua/vm/types.h>


constexpr auto VIUA_SLOW_CYCLES = false;

namespace viua {
auto TRACE_STREAM = viua::support::fdstream{2};
}


namespace {
auto run_instruction(viua::vm::Stack& stack)
    -> viua::arch::instruction_type const*
{
    auto instruction = viua::arch::instruction_type{};
    do {
        instruction = *stack.ip;
        stack.ip    = viua::vm::ins::execute(stack, stack.ip);
        ++stack.proc.core->perf_counters.total_ops_executed;
    } while ((stack.ip != nullptr) and (instruction & viua::arch::ops::GREEDY));

    return stack.ip;
}

auto format_time(std::chrono::microseconds const us) -> std::string
{
    auto out = std::ostringstream{};
    out << std::fixed << std::setprecision(2);
    if (us.count() > 1e6) {
        out << (us.count() / 1.0e6) << "s";
    } else if (us.count() > 1e3) {
        out << (us.count() / 1.0e3) << "ms";
    } else {
        out << us.count() << "us";
    }
    return out.str();
}
auto format_hz(uint64_t const hz) -> std::string
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
auto run(viua::vm::Process& proc) -> bool
{
    auto const ip_ok = [&proc]() -> bool {
        return proc.module.ip_in_valid_range(proc.stack.ip);
    };

    if constexpr (viua::vm::ins::VIUA_TRACE_CYCLES) {
        viua::TRACE_STREAM << "cycle at " << proc.module.elf_path.native()
                           << "[.text+0x" << std::hex << std::setw(16)
                           << std::setfill('0')
                           << ((proc.stack.ip - proc.module.ip_base)
                               * sizeof(viua::arch::instruction_type))
                           << std::dec << "] in process "
                           << proc.pid.to_string() << viua::TRACE_STREAM.endl;
    }

    constexpr auto PREEMPTION_THRESHOLD = size_t{2};
    for (auto i = size_t{0}; i < PREEMPTION_THRESHOLD and ip_ok(); ++i) {
        /*
         * This is needed to detect greedy bundles and adjust preemption
         * counter appropriately. If a greedy bundle contains more
         * instructions than the preemption threshold allows the process
         * will be suspended immediately.
         */
        auto const greedy    = (*proc.stack.ip & viua::arch::ops::GREEDY);
        auto const bundle_ip = proc.stack.ip;

        proc.stack.ip = run_instruction(proc.stack);

        /*
         * If the instruction was a greedy bundle instead of a single
         * one, the preemption counter has to be adjusted. It may be the
         * case that the bundle already hit the preemption threshold.
         */
        if (greedy and ip_ok()) {
            i += (proc.stack.ip - bundle_ip) - 1;
        }
    }

    if (proc.stack.frames.empty()) {
        std::cerr << "[vm:sched:proc] process " << proc.pid.to_string()
                  << " has empty stack\n";
        return false;
    }
    if (not ip_ok()) {
        std::cerr << "[vm] ip " << std::hex << std::setw(8) << std::setfill('0')
                  << ((proc.stack.ip - proc.module.ip_base)
                      * sizeof(viua::arch::instruction_type))
                  << std::dec << " outside of valid range\n";
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
auto run(viua::vm::Core& core) -> void
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
        auto const approx_hz = (1e6 / total_us.count()) * total_ops;
        viua::TRACE_STREAM << "[vm:perf] executed ops " << total_ops
                           << ", run time " << format_time(total_us)
                           << viua::TRACE_STREAM.endl;
        viua::TRACE_STREAM << "[vm:perf] approximate frequency "
                           << format_hz(approx_hz) << viua::TRACE_STREAM.endl;
    }
}
}  // namespace

auto main(int argc, char* argv[]) -> int
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    auto const args = std::vector<std::string>{(argv + 1), (argv + argc)};
    if (args.empty()) {
        std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": no executable to run\n";
        return 1;
    }

    auto verbosity_level = 0;
    auto show_version    = false;
    auto show_help       = false;

    for (auto i = decltype(args)::size_type{}; i < args.size(); ++i) {
        auto const& each = args.at(i);
        if (each == "--") {
            // explicit separator of options and operands
            break;
        }
        /*
         * Common options.
         */
        else if (each == "-v" or each == "--verbose") {
            ++verbosity_level;
        } else if (each == "--version") {
            show_version = true;
        } else if (each == "--help") {
            show_help = true;
        } else if (each.front() == '-') {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": unknown option: " << each << "\n";
            return 1;
        } else {
            // input files start here
            break;
        }
    }

    if (show_version) {
        if (verbosity_level) {
            std::cout << "Viua VM ";
        }
        std::cout << (verbosity_level ? VIUAVM_VERSION_FULL : VIUAVM_VERSION)
                  << "\n";
        return 0;
    }
    if (show_help) {
        if (execlp("man", "man", "1", "viua-vm", nullptr) == -1) {
            perror("WTF");
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": man(1) page not installed or not found\n";
            return 1;
        }
    }

    /*
     * Do not assume that the path given by the user points to a file that
     * exists. Typos are a thing. And let's check if the file really is a
     * regular file - trying to execute directories or device files does not
     * make much sense.
     */
    auto const elf_path = std::filesystem::path{args.back()};
    if (not std::filesystem::exists(elf_path)) {
        std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": file does not exist: " << esc(2, COLOR_FG_WHITE)
                  << elf_path.native() << esc(2, ATTR_RESET) << "\n";
        return 1;
    }
    {
        struct stat statbuf {
        };
        if (stat(elf_path.c_str(), &statbuf) == -1) {
            auto const saved_errno = errno;
            auto const errname     = strerrorname_np(saved_errno);
            auto const errdesc     = strerrordesc_np(saved_errno);

            std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                      << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                      << esc(2, ATTR_RESET);
            if (errname) {
                std::cerr << ": " << errname;
            }
            std::cerr << ": " << (errdesc ? errdesc : "unknown error") << "\n";
            return 1;
        }
        if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
            std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                      << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                      << esc(2, ATTR_RESET);
            std::cerr << ": not a regular file\n";
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
        auto const errname     = strerrorname_np(saved_errno);
        auto const errdesc     = strerrordesc_np(saved_errno);

        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET);
        if (errname) {
            std::cerr << ": " << errname;
        }
        std::cerr << ": " << (errdesc ? errdesc : "unknown error") << "\n";
        return 1;
    }

    using Module           = viua::vm::elf::Loaded_elf;
    auto const main_module = Module::load(elf_fd);

    if (auto const f = main_module.find_fragment(".rodata");
        not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": no strings fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .rodata section found\n";
        return 1;
    }
    if (auto const f = main_module.find_fragment(".viua.fns");
        not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET)
                  << ": no function table fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .viua.fns section found\n";
        return 1;
    }
    if (auto const f = main_module.find_fragment(".text"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": no text fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .text section found\n";
        return 1;
    }

    auto entry_addr = size_t{0};
    if (auto const ep = main_module.entry_point(); ep.has_value()) {
        entry_addr = (*ep / sizeof(viua::arch::instruction_type));
    } else {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": no entry point defined\n";
        return 1;
    }

    auto core = viua::vm::Core{};
    core.modules.emplace("", viua::vm::Module{elf_path, main_module});
    auto const main_pid [[maybe_unused]] = core.spawn("", entry_addr);

    if constexpr (viua::vm::ins::VIUA_TRACE_CYCLES) {
        if (auto trace_fd = getenv("VIUA_VM_TRACE_FD"); trace_fd) {
            try {
                /*
                 * Assume an file descriptor opened for writing was received.
                 */
                viua::TRACE_STREAM =
                    viua::support::fdstream{std::stoi(trace_fd)};
            } catch (std::invalid_argument const&) {
                /*
                 * Otherwise, treat the thing received as a filename and open it
                 * for writing.
                 */
                viua::TRACE_STREAM = viua::support::fdstream{
                    open(trace_fd, O_WRONLY | O_CLOEXEC)};
            }
        }
    }

    try {
        run(core);
    } catch (viua::vm::abort_execution const& e) {
        std::cerr << "Aborted execution: " << e.what() << "\n";
        if constexpr (true) {
            throw;
        } else {
            return 1;
        }
    }

    return 0;
}
