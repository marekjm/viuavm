/*
 *  Copyright (C) 2022 Marek Marecki
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

#include <fcntl.h>
#include <sys/types.h>

#include <experimental/memory>
#include <filesystem>
#include <memory>
#include <iostream>
#include <string>
#include <vector>

#include <linenoise/encodings/utf8.h>
#include <linenoise/linenoise.h>

#include <viua/arch/ops.h>
#include <viua/arch/ins.h>
#include <viua/vm/core.h>
#include <viua/vm/ins.h>
#include <viua/support/fdstream.h>
#include <viua/support/tty.h>


struct Global_state {
    viua::vm::Core core {};

    using pid_type = viua::runtime::PID;
    std::unique_ptr<pid_type> selected_pid;
    std::optional<size_t> selected_frame;

    std::string last_input;
};


namespace {
auto REPL_STATE = std::experimental::observer_ptr<Global_state>{};
}


/*
 * Utility namespace.
 */
namespace {
auto split_on_space(std::string_view sv) -> std::vector<std::string_view>
{
    auto parts = std::vector<std::string_view>{};

    while (not sv.empty()) {
        auto const space = sv.find(' ');

        if (space == std::string_view::npos) {
            parts.push_back(sv);
            break;
        }

        if (space != 0) {
            parts.push_back(std::string_view{sv.data(), space});
        }

        sv.remove_prefix(space + 1);
    }

    return parts;
}
}

auto completion(char const* buf, linenoiseCompletions* const lc) -> void
{
    static std::vector<std::string> candidates;
    candidates.clear();

    candidates.push_back("quit");
    candidates.push_back("repl");
    candidates.push_back("repl pid-base");
    candidates.push_back("actor");
    candidates.push_back("actor new");
    candidates.push_back("load");
    candidates.push_back("backtrace");
    candidates.push_back("stepi");

    for (auto const& each : candidates) {
        if (not each.starts_with(buf)) {
            continue;
        }

        linenoiseAddCompletion(lc, each.c_str());
    }

    auto const sv = std::string_view{buf};
    auto const parts = split_on_space(sv);
    if (parts.empty()) {
        return;
    }

    auto const p = [&parts](size_t const n) -> std::optional<std::string_view>
    {
        return (n < parts.size()) ? std::optional{parts.at(n)} : std::nullopt;
    };

    static std::vector<std::string> runtime_candidates;
    runtime_candidates.clear();

    /*
     * Provide completions for shortcut commands.
     */
    {
        if (sv == "bt") {
            runtime_candidates.push_back("backtrace");
        }
    }

    auto const should_complete_files_for_load = (
        (*p(0) == "load")
        and p(1).has_value()
        and (
            p(2).has_value() or ((not p(2).has_value()) and sv.back() == ' ')
        )
    );
    if (should_complete_files_for_load) {
        namespace fs = std::filesystem;

        /*
         * Raw path may already point to a regular file, and this possibility
         * MUST be checked first. If nothing was supplied assume the user wants
         * to complete over all files in the current directory.
         */
        auto const raw = fs::path{p(2).value_or(".")};

        /*
         * If the path was given as "bar/foo" where "foo" is the beginning of a
         * longer file name we will need to iterate over all files beginning
         * with "foo" in the parent directory of supplied path (which is "bar"
         * in the example case).
         */
        auto const stem = raw.stem();

        /*
         * The parent directory of supplied path is a little bit tricky to get.
         * Since it can produce an empty (and thus invalid) path for some
         * strings (notably for "foo" ie, a filename without any slashes) we
         * need to detect this case and force-feed the parent to be . ie, the
         * current directory. Otherwise the completion behaviour is completely
         * broken.
         */
        auto const parent = (raw.parent_path().string().empty()
                ? fs::path{"."}
                : raw.parent_path());

        auto const prefix = "load " + std::string{*p(1)} + ' ';
        /* runtime_candidates.push_back(prefix + raw.string() + " (raw)"); */
        /* runtime_candidates.push_back(prefix + parent.string() + " (parent)"); */
        /* runtime_candidates.push_back(prefix + stem.string() + " (stem)"); */

        if (fs::exists(raw) and fs::is_regular_file(raw)) {
            /*
             * Do nothing, as the part already represents a valid path to a
             * file.
             */
        } else if (fs::exists(raw) and fs::is_directory(raw)) {
            for (auto& p : fs::directory_iterator{raw}) {
                runtime_candidates.push_back(
                    prefix
                    + p.path().native()
                    + (fs::is_directory(p.path()) ? "/" : ""));
            }
        } else if (fs::exists(parent) and fs::is_directory(parent)) {
            for (auto& p : fs::directory_iterator{parent}) {
                if (not p.path().stem().string().starts_with(stem.string())) {
                    continue;
                }
                runtime_candidates.push_back(
                    prefix
                    + p.path().native()
                    + (fs::is_directory(p.path()) ? "/" : ""));
            }
        }
    }

    if (*p(0) == "actor" and p(1).has_value() and *p(1) == "new") {
        auto const stem = p(2).value_or("");
        auto const prefix = std::string{"actor new "};
        for (auto const& [ mod_name, mod ] : REPL_STATE->core.modules) {
            for (auto const& [ fn_off, fn ] : mod.elf.function_table()) {
                auto const fn_id = (mod_name.empty() ? "" : (mod_name + "::")) + std::get<0>(fn);
                if (fn_id.starts_with(stem)) {
                    runtime_candidates.push_back(prefix + fn_id);
                }
            }
        }
    }

    for (auto const& each : runtime_candidates) {
        linenoiseAddCompletion(lc, each.c_str());
    }
}

auto hints_impl(char const* buf, int* const color, int* const bold) -> char const*
{
    static_cast<void>(buf);
    static_cast<void>(color);
    static_cast<void>(bold);

    return nullptr;
}
auto hints(char const* buf, int* const color, int* const bold) -> char*
{
    return const_cast<char*>(hints_impl(buf, color, bold));
}

namespace viua {
auto TRACE_STREAM = viua::support::fdstream{2};
}

/*
 * This is the symbol users should use when loading a module to be
 * designated as the main module.
 */
constexpr auto MAIN_MODULE_MNEMONIC = "main";
auto load_module(std::string_view const name, std::filesystem::path elf_path)
    -> void
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    /*
     * Do not assume that the path given by the user points to a file that
     * exists. Typos are a thing. And let's check if the file really is a
     * regular file - trying to execute directories or device files does not
     * make much sense.
     */
    if (not std::filesystem::exists(elf_path)) {
        std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": file does not exist: " << esc(2, COLOR_FG_WHITE)
                  << elf_path.native() << esc(2, ATTR_RESET) << "\n";
        return;
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
            return;
        }
        if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
            std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                      << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                      << esc(2, ATTR_RESET);
            std::cerr << ": not a regular file\n";
            return;
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
        return;
    }

    using Module   = viua::vm::elf::Loaded_elf;
    auto const mod = Module::load(elf_fd);

    if (auto const f = mod.find_fragment(".rodata"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": no strings fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .rodata section found\n";
        return;
    }
    if (auto const f = mod.find_fragment(".viua.fns"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET)
                  << ": no function table fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .viua.fns section found\n";
        return;
    }
    if (auto const f = mod.find_fragment(".text"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": no text fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .text section found\n";
        return;
    }

    if (auto const ep = mod.entry_point(); ep.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native() << ": "
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET)
                  << ": an entry point is defined for this module\n";
    }

    REPL_STATE->core.modules.emplace(
        ((name == MAIN_MODULE_MNEMONIC) ? "" : name),
        viua::vm::Module{elf_path, mod});
}

auto repl_eval(std::vector<std::string_view> const parts) -> bool
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    auto const p = [&parts](size_t const n) -> std::optional<std::string_view> {
        return (n < parts.size()) ? std::optional{parts.at(n)} : std::nullopt;
    };

    if (*p(0) == "quit") {
        return false;
    } else if (*p(0) == "repl") {
        if (*p(1) == "pid-base") {
            using viua::runtime::PID;
            std::cout << PID{REPL_STATE->core.pids.base}.to_string() << "\n\r";
        }
    } else if (*p(0) == "load") {
        auto const name     = *p(1);
        auto const elf_path = std::filesystem::path{*p(2)};
        load_module(name, elf_path);
    } else if (*p(0) == "actor") {
        if (*p(1) == "new" and p(2).has_value()) {
            if (REPL_STATE->core.modules.empty()) {
                std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                    << ": no modules loaded\n";
                return true;
            }

            auto const fn_id = *p(2);
            auto const mod_name = std::string{(fn_id.rfind("::") == std::string::npos)
                ? ""
                : fn_id.substr(0, fn_id.rfind("::"))};
            auto const fn_name = (fn_id.rfind("::") == std::string::npos)
                ? fn_id
                : fn_id.substr(fn_id.rfind("::") + 2);

            if (not REPL_STATE->core.modules.count(mod_name)) {
                std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                    << ": module "
                    << esc(2, COLOR_FG_WHITE) << mod_name << esc(2, ATTR_RESET)
                    << " does not exist\n";
                return true;
            }
            auto const& mod = REPL_STATE->core.modules.at(mod_name);

            for (auto const& each : mod.elf.function_table()) {
                if (fn_name != std::get<0>(std::get<1>(each))) {
                    continue;
                }

                auto const fn_addr = (std::get<1>(std::get<1>(each)) / sizeof(viua::arch::instruction_type));
                auto pid = REPL_STATE->core.spawn(mod_name, fn_addr);
                REPL_STATE->selected_pid = std::make_unique<viua::runtime::PID>(pid);
            }
        }
    } else if (*p(0) == "backtrace" or *p(0) == "bt") {
        if (not REPL_STATE->selected_pid) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": no selected actor\n";
            return true;
        }

        auto const proc = REPL_STATE->core.find(*REPL_STATE->selected_pid);
        if (not proc) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": actor " << esc(2, COLOR_FG_WHITE)
                      << REPL_STATE->selected_pid->to_string()
                      << esc(2, ATTR_RESET) << " does not exist\n\r";
            return true;
        }

        if (proc->stack.frames.empty()) {
            std::cerr << "stack empty\n\r";
        } else {
            viua::vm::ins::print_backtrace(proc->stack);
        }
    } else if (*p(0) == "show") {
        if (p(1).value_or("") == "frame") {
            if (not REPL_STATE->selected_pid) {
                std::cerr << esc(2, COLOR_FG_RED) << "error"
                          << esc(2, ATTR_RESET) << ": no selected actor\n";
                return true;
            }

            auto const proc = REPL_STATE->core.find(*REPL_STATE->selected_pid);
            if (not proc) {
                std::cerr << esc(2, COLOR_FG_RED) << "error"
                          << esc(2, ATTR_RESET) << ": actor "
                          << esc(2, COLOR_FG_WHITE)
                          << REPL_STATE->selected_pid->to_string()
                          << esc(2, ATTR_RESET) << " does not exist\n\r";
                return true;
            }

            auto const user_frame_index =
                (p(2).has_value() ? std::stoull(std::string{*p(2)})
                                  : REPL_STATE->selected_frame.value_or(0));
            if (user_frame_index >= proc->stack.frames.size()) {
                std::cerr << esc(2, COLOR_FG_RED) << "error"
                          << esc(2, ATTR_RESET) << ": frame "
                          << user_frame_index << " does not exist\n\r";
                return true;
            }

            auto const physical_frame_index =
                proc->stack.frames.size() - user_frame_index - 1;
            auto const& frame = proc->stack.frames.at(physical_frame_index);
            viua::vm::ins::print_backtrace(proc->stack, physical_frame_index);
            viua::vm::ins::dump_registers(frame.parameters, "p");
            viua::vm::ins::dump_registers(frame.registers, "l");
            viua::vm::ins::dump_registers(proc->stack.args, "a");
        }
    } else if (*p(0) == "stepi") {
        if (not REPL_STATE->selected_pid) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                << ": no selected actor\n";
            return true;
        }

        auto const proc = REPL_STATE->core.find(*REPL_STATE->selected_pid);
        if (not proc) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                << ": actor "
                << esc(2, COLOR_FG_WHITE) << REPL_STATE->selected_pid->to_string()
                << esc(2, ATTR_RESET) << " does not exist\n\r";
            return true;
        }

        auto const limit = std::stoull(std::string{p(1).value_or("1")});

        REPL_STATE->selected_frame.reset();

        try {
            for (auto i = size_t{0}; i < limit; ++i) {
                proc->stack.ip = viua::vm::ins::execute(proc->stack, proc->stack.ip);
            }
        } catch (viua::vm::abort_execution const& e) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                << ": aborted execution: " << e.what() << "\n\r";
            return true;
        }
    } else if (*p(0) == "step") {
        if (not REPL_STATE->selected_pid) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                << ": no selected actor\n";
            return true;
        }

        auto const proc = REPL_STATE->core.find(*REPL_STATE->selected_pid);
        if (not proc) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                << ": actor "
                << esc(2, COLOR_FG_WHITE) << REPL_STATE->selected_pid->to_string()
                << esc(2, ATTR_RESET) << " does not exist\n\r";
            return true;
        }

        auto const limit = std::stoull(std::string{p(1).value_or("1")});

        REPL_STATE->selected_frame.reset();

        try {
            for (auto i = size_t{0}; i < limit; ++i) {
                auto instruction = viua::arch::instruction_type{};
                do {
                    instruction = *proc->stack.ip;
                    proc->stack.ip    = viua::vm::ins::execute(proc->stack, proc->stack.ip);
                } while ((proc->stack.ip != nullptr) and (instruction & viua::arch::ops::GREEDY));
            }
        } catch (viua::vm::abort_execution const& e) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                << ": aborted execution: " << e.what() << "\n\r";
            return true;
        }
    } else if (*p(0) == "up") {
        if (not REPL_STATE->selected_pid) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": no selected actor\n";
            return true;
        }

        auto const proc = REPL_STATE->core.find(*REPL_STATE->selected_pid);
        if (not proc) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": actor " << esc(2, COLOR_FG_WHITE)
                      << REPL_STATE->selected_pid->to_string()
                      << esc(2, ATTR_RESET) << " does not exist\n\r";
            return true;
        }

        auto const user_frame_index =
            REPL_STATE->selected_frame.value_or(0) + 1;
        if (user_frame_index >= proc->stack.frames.size()) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": frame " << user_frame_index
                      << " does not exist\n\r";
            return true;
        }

        REPL_STATE->selected_frame = user_frame_index;
        auto const physical_frame_index =
            proc->stack.frames.size() - user_frame_index - 1;
        viua::vm::ins::print_backtrace(proc->stack, physical_frame_index);
    } else if (*p(0) == "down") {
        if (not REPL_STATE->selected_pid) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": no selected actor\n";
            return true;
        }

        auto const proc = REPL_STATE->core.find(*REPL_STATE->selected_pid);
        if (not proc) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": actor " << esc(2, COLOR_FG_WHITE)
                      << REPL_STATE->selected_pid->to_string()
                      << esc(2, ATTR_RESET) << " does not exist\n\r";
            return true;
        }

        auto const user_frame_index =
            REPL_STATE->selected_frame.value_or(0) - 1;
        if (user_frame_index >= proc->stack.frames.size()) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": frame " << user_frame_index
                      << " does not exist\n\r";
            return true;
        }

        REPL_STATE->selected_frame = user_frame_index;
        auto const physical_frame_index =
            proc->stack.frames.size() - user_frame_index - 1;
        viua::vm::ins::print_backtrace(proc->stack, physical_frame_index);
    }

    return true;
}
auto repl_main() -> void
{
    constexpr auto DEFAULT_PROMPT = "(viua) ";

    auto raw_line = static_cast<char*>(nullptr);
    while ((raw_line = linenoise(DEFAULT_PROMPT))) {
        linenoiseHistoryAdd(raw_line);
        auto const line = std::string{raw_line};
        free(raw_line);

        auto const useful_line = std::string_view{line.empty() ? REPL_STATE->last_input : line};
        if (auto const parts = split_on_space(useful_line); not parts.empty()) {
            if (not repl_eval(parts)) {
                break;
            }
            REPL_STATE->last_input = useful_line;
        }
    }
}

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

    auto args            = std::vector<std::string>{(argv + 1), (argv + argc)};
    auto verbosity_level = 0;
    {
        auto show_version = false;

        auto i = decltype(args)::size_type{};
        for (; i < args.size(); ++i) {
            auto const& each = args.at(i);
            if (each == "--") {
                // explicit separator of options and operands
                ++i;
                break;
            }

            /*
             * Common options.
             */
            else if (each == "-v" or each == "--verbose") {
                ++verbosity_level;
            } else if (each == "--version") {
                show_version = true;
            } else if (each.front() == '-') {
                // unknown option
            } else {
                // input files start here
                break;
            }
        }

        args = std::vector<std::string>{args.begin() + i, args.end()};

        if (show_version) {
            if (verbosity_level) {
                std::cout << "Viua VM ";
            }
            std::cout
                << (verbosity_level ? VIUAVM_VERSION_FULL : VIUAVM_VERSION)
                << "\n";
            return 0;
        }
    }

    {
        /*
         * Enable UTF-8 support. Without this the console may act "funny" when it
         * encounters multi-byte characters.
         */
        linenoiseSetEncodingFunctions(linenoiseUtf8PrevCharLen,
                                      linenoiseUtf8NextCharLen,
                                      linenoiseUtf8ReadCode);

        /* Set the completion callback. This will be called every time the
         * user uses the <tab> key. */
        linenoiseSetCompletionCallback(completion);
        linenoiseSetHintsCallback(hints);
    }

    std::cout << esc(1, COLOR_FG_WHITE) << "Viua REPL (debugger) "
        << esc(1, ATTR_RESET) << VIUAVM_VERSION << "\n";

    auto state = std::make_unique<Global_state>();
    REPL_STATE = std::experimental::make_observer(state.get());

    if (not args.empty()) {
        load_module(MAIN_MODULE_MNEMONIC, args.front());
    }

    repl_main();

    return 0;
}
