/*
 *  Copyright (C) 2022, 2025 Marek Marecki
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

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <linenoise/encodings/utf8.h>
#include <linenoise/linenoise.h>

#include <viua/arch/arch.h>
#include <viua/arch/ins.h>
#include <viua/arch/ops.h>
#include <viua/libexec/common.hh>
#include <viua/support/errno.h>
#include <viua/support/fdstream.h>
#include <viua/support/memory.h>
#include <viua/support/number.h>
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


struct Breakpoint {
    size_t offset;
    std::optional<std::string> symbol;

    auto to_string() const -> std::string;
};
auto Breakpoint::to_string() const -> std::string
{
    return std::format(
        "[.text+0x{:016x}]{}",
        (offset * sizeof(viua::arch::instruction_type)),
        symbol.transform([](auto const s) { return " " + s; }).value_or(""));
}

struct Interpreter_state {
    std::unique_ptr<viua::vm::Core> core{};

    using pid_type = viua::runtime::PID;
    std::unique_ptr<pid_type> selected_pid;
    std::optional<size_t> selected_frame;

    std::vector<Breakpoint> breakpoints;

    std::string last_input;
    bool crash_on_internal{ false };

    using io_type = viua::vm::io::impl::VIUAVM_IO_IMPL::IO;
    Interpreter_state(io_type&);
};

Interpreter_state::Interpreter_state(
    io_type& io)
    : core{ std::make_unique<viua::vm::Core>(io) }
{}

namespace {
auto REPL_STATE = viua::view_ptr<Interpreter_state>{};
}

/*
 * Utility namespace.
 */
namespace {
auto split_on_space(
    std::string_view sv) -> std::vector<std::string_view>
{
    auto parts = std::vector<std::string_view>{};

    while (not sv.empty()) {
        auto const space = sv.find(' ');

        if (space == std::string_view::npos) {
            parts.push_back(sv);
            break;
        }

        if (space != 0) {
            parts.push_back(std::string_view{ sv.data(), space });
        }

        sv.remove_prefix(space + 1);
    }

    return parts;
}
}  // namespace

auto completion(
    char const* buf,
    linenoiseCompletions* const lc) -> void
{
    static std::vector<std::string> candidates;
    candidates.clear();

    candidates.push_back("quit");
#if 0
    candidates.push_back("repl");
    candidates.push_back("repl pid-base");
    candidates.push_back("repl abort-internal");
    candidates.push_back("repl abort-internal true");
    candidates.push_back("repl abort-internal false");
    candidates.push_back("actor");
    candidates.push_back("actor new");
    candidates.push_back("load");
    candidates.push_back("backtrace");
    candidates.push_back("stepi");
    candidates.push_back("stepi.g");
    candidates.push_back("show frame");
    candidates.push_back("show ip");
    candidates.push_back("up");
    candidates.push_back("down");
    candidates.push_back("eval");
    candidates.push_back("eval asm");
#endif

    for (auto const& each : candidates) {
        if (not each.starts_with(buf)) {
            continue;
        }

        linenoiseAddCompletion(lc, each.c_str());
    }

#if 0
    auto const sv    = std::string_view{ buf };
    auto const parts = split_on_space(sv);
    if (parts.empty()) {
        return;
    }

    auto const p = [&parts](size_t const n) -> std::optional<std::string_view>
    {
        return (n < parts.size()) ? std::optional{ parts.at(n) } : std::nullopt;
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

    auto const should_complete_files_for_load =
        ((*p(0) == "load") and p(1).has_value()
         and (p(2).has_value()
              or ((not p(2).has_value()) and sv.back() == ' ')));
    if (should_complete_files_for_load) {
        namespace fs = std::filesystem;

        /*
         * Raw path may already point to a regular file, and this possibility
         * MUST be checked first. If nothing was supplied assume the user wants
         * to complete over all files in the current directory.
         */
        auto const raw = fs::path{ p(2).value_or(".") };

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
        auto const parent =
            (raw.parent_path().string().empty() ? fs::path{ "." }
                                                : raw.parent_path());

        auto const prefix = "load " + std::string{ *p(1) } + ' ';

        if (fs::exists(raw) and fs::is_regular_file(raw)) {
            /*
             * Do nothing, as the part already represents a valid path to a
             * file.
             */
        } else if (fs::exists(raw) and fs::is_directory(raw)) {
            for (auto& p : fs::directory_iterator{ raw }) {
                runtime_candidates.push_back(
                    prefix + p.path().native()
                    + (fs::is_directory(p.path()) ? "/" : ""));
            }
        } else if (fs::exists(parent) and fs::is_directory(parent)) {
            for (auto& p : fs::directory_iterator{ parent }) {
                if (not p.path().stem().string().starts_with(stem.string())) {
                    continue;
                }
                runtime_candidates.push_back(
                    prefix + p.path().native()
                    + (fs::is_directory(p.path()) ? "/" : ""));
            }
        }
    }

    if (*p(0) == "actor" and p(1).has_value() and *p(1) == "new") {
        auto const stem   = p(2).value_or("");
        auto const prefix = std::string{ "actor new " };
        for (auto const& [mod_name, mod] : REPL_STATE->core->modules) {
            for (auto const& [fn_off, fn] : mod.elf.function_table()) {
                auto const fn_id = (mod_name.empty() ? "" : (mod_name + "::"))
                                   + std::get<0>(fn);
                if (fn_id.starts_with(stem)) {
                    runtime_candidates.push_back(prefix + fn_id);
                }
            }
        }
    }

    for (auto const& each : runtime_candidates) {
        linenoiseAddCompletion(lc, each.c_str());
    }
#endif
}

auto hints_impl [[maybe_unused]] (
    char const* buf,
    int* const color,
    int* const bold) -> char const*
{
    static_cast<void>(buf);
    static_cast<void>(color);
    static_cast<void>(bold);

    return nullptr;
}
auto hints [[maybe_unused]] (
    char const* buf,
    int* const color,
    int* const bold) -> char*
{
    return const_cast<char*>(hints_impl(buf, color, bold));
}

namespace viua {
auto TRACE_STREAM = viua::support::fdstream{ 2 };
}

/*
 * This is the symbol users should use when loading a module to be
 * designated as the main module.
 */
constexpr auto MAIN_MODULE_MNEMONIC = "main";


/*
 * Load a module, mapping a file path to a module name in the interpreter's
 * global state.
 *
 * The function returns true when it encounters an error, allowing for the
 * following exit early-style pattern:
 *
 *      if (load_module(...)) {
 *          report_error(...);
 *      }
 */
auto load_module(
    std::string_view const name,
    std::filesystem::path elf_path) -> bool
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
        viua::support::errorln("file does not exist: {}{}{}",
                               esc(2, COLOR_FG_WHITE),
                               elf_path.native(),
                               esc(2, ATTR_RESET));
        return true;
    }
    {
        struct stat statbuf{};
        if (stat(elf_path.c_str(), &statbuf) == -1) {
            auto const saved_errno = errno;
            auto const errname     = viua::support::errno_name(saved_errno);
            auto const errdesc     = viua::support::errno_desc(saved_errno);

            viua::support::errorln(elf_path, "{}: {}", errname, errdesc);
            return true;
        }
        if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
            viua::support::errorln(elf_path, "not a regular file");
            return true;
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
        return true;
    }

    using Module   = viua::vm::elf::Loaded_elf;
    auto const mod = Module::load(elf_fd);

    if (auto const f = mod.find_fragment(".rodata"); not f.has_value()) {
        viua::support::errorln(elf_path, "no strings fragment found");
        viua::support::noteln(elf_path, "no .rodata section found");
        return true;
    }
    if (auto const f = mod.find_fragment(".symtab"); not f.has_value()) {
        viua::support::errorln(elf_path, "no function table fragment found");
        viua::support::noteln(elf_path, "no .symtab section found");
        return true;
    }
    if (auto const f = mod.find_fragment(".strtab"); not f.has_value()) {
        viua::support::errorln(elf_path, "no string table fragment found");
        viua::support::noteln(elf_path, "no .strtab section found");
        return true;
    }
    if (auto const f = mod.find_fragment(".text"); not f.has_value()) {
        viua::support::errorln(elf_path, "no text fragment found");
        viua::support::noteln(elf_path, "no .text section found");
        return true;
    }

    if (auto const ep = mod.entry_point(); ep.has_value()) {
        viua::support::noteln(
            elf_path, "an entry point is defined for this module");
    }

    REPL_STATE->core->modules.emplace(
        ((name == MAIN_MODULE_MNEMONIC) ? "" : name),
        viua::vm::Module{ elf_path, mod });

    return false;
}

#if 0
auto evaluate_asm_expression(
    std::string const source_text) -> void
{
    auto lexemes = std::vector<viua::libs::lexer::Lexeme>{};
    try {
        lexemes = viua::libs::lexer::lex(source_text);
        lexemes = viua::libs::parser::ast::remove_noise(std::move(lexemes));
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error("-", source_text, e);
        return;
    }

    auto lv = viua::support::vector_view{ lexemes };
    auto p  = viua::libs::parser::ast::Instruction{};
    try {
        p = viua::libs::parser::parse_instruction(lv);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error("-", source_text, e);
        return;
    }

    auto fn_offsets = std::map<std::string, size_t>{};
    {
        for (auto const& [mod_name, mod] : REPL_STATE->core.modules) {
            for (auto const& [fn_off, fn] : mod.elf.function_table()) {
                auto const fn_id = (mod_name.empty() ? "" : (mod_name + "::"))
                                   + std::get<0>(fn);
                fn_offsets[fn_id] = fn_off;
            }
        }
    }

    auto strings_table = std::vector<uint8_t>{};
    auto var_offsets   = std::map<std::string, size_t>{};
    auto cooked        = std::vector<viua::libs::parser::ast::Instruction>{};
    try {
        cooked = viua::libs::stage::cook_long_immediates(
            p, strings_table, var_offsets);
        cooked =
            viua::libs::stage::expand_pseudoinstructions(cooked, fn_offsets);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error("-", source_text, e);
        return;
    }

    auto instructions = std::vector<viua::arch::instruction_type>{};
    for (auto const& each : cooked) {
        try {
            auto const i = viua::libs::stage::emit_instruction(each);
            instructions.push_back(i);
        } catch (viua::libs::errors::compile_time::Error const& e) {
            viua::libs::stage::display_error("-", source_text, e);
            return;
        }
    }

    auto proc = REPL_STATE->core.find(*REPL_STATE->selected_pid);

    proc->strtab = &strings_table;

    for (auto const each : instructions) {
        viua::vm::ins::execute(proc->stack, &each);
    }

    proc->strtab = &proc->module.strings_table;
}

auto repl_eval(
    std::vector<std::string_view> const parts) -> bool
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    auto const p = [&parts](size_t const n) -> std::optional<std::string_view>
    {
        return (n < parts.size()) ? std::optional{ parts.at(n) } : std::nullopt;
    };

    if (*p(0) == "quit") {
        return false;
    } else if (*p(0) == "repl") {
        if (*p(1) == "pid-base") {
            using viua::runtime::PID;
            std::cout << PID{ REPL_STATE->core.pids.base }.to_string()
                      << "\n\r";
        } else if (*p(1) == "abort-internal") {
            if (*p(2) == "true") {
                REPL_STATE->crash_on_internal = true;
            } else if (*p(2) == "false") {
                REPL_STATE->crash_on_internal = false;
            } else {
                std::cout << REPL_STATE->crash_on_internal << "\n\r";
            }
        }
    } else if (*p(0) == "load") {
        auto const name     = *p(1);
        auto const elf_path = std::filesystem::path{ *p(2) };
        load_module(name, elf_path);
    } else if (*p(0) == "actor") {
        if (*p(1) == "new" and p(2).has_value()) {
            if (REPL_STATE->core.modules.empty()) {
                std::cerr << esc(2, COLOR_FG_RED) << "error"
                          << esc(2, ATTR_RESET) << ": no modules loaded\n";
                return true;
            }

            auto const fn_id = *p(2);
            auto const mod_name =
                std::string{ (fn_id.rfind("::") == std::string::npos)
                                 ? ""
                                 : fn_id.substr(0, fn_id.rfind("::")) };
            auto const fn_name = (fn_id.rfind("::") == std::string::npos)
                                     ? fn_id
                                     : fn_id.substr(fn_id.rfind("::") + 2);

            if (not REPL_STATE->core.modules.count(mod_name)) {
                std::cerr << esc(2, COLOR_FG_RED) << "error"
                          << esc(2, ATTR_RESET) << ": module "
                          << esc(2, COLOR_FG_WHITE) << mod_name
                          << esc(2, ATTR_RESET) << " does not exist\n";
                return true;
            }
            auto const& mod = REPL_STATE->core.modules.at(mod_name);

            for (auto const& each : mod.elf.function_table()) {
                if (fn_name != std::get<0>(std::get<1>(each))) {
                    continue;
                }

                auto const fn_addr = (std::get<1>(std::get<1>(each))
                                      / sizeof(viua::arch::instruction_type));
                auto pid           = REPL_STATE->core.spawn(mod_name, fn_addr);
                REPL_STATE->selected_pid =
                    std::make_unique<viua::runtime::PID>(pid);

                break;
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
                (p(2).has_value() ? std::stoull(std::string{ *p(2) })
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
            viua::vm::ins::dump_registers(frame.parameters, proc->atoms, "p");
            viua::vm::ins::dump_registers(frame.registers, proc->atoms, "l");
            viua::vm::ins::dump_registers(proc->stack.args, proc->atoms, "a");
            viua::vm::ins::dump_memory(proc->memory);
        } else if (p(1).value_or("") == "ip") {
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

            auto const& module = proc->module;
            std::cout << "  " << std::hex << std::setfill(' ') << std::setw(16)
                      << proc->stack.ip << " " << module.elf_path.native()
                      << "[.text+0x" << std::hex << std::setfill('0')
                      << std::setw(16) << (proc->stack.ip - module.ip_base)
                      << "]"
                      << "\n";
            std::cout
                << "  " << std::string(16, ' ') << " "
                << module.elf_path.native() << "[.text+0x" << std::hex
                << std::setfill('0') << std::setw(16) << unsigned{ 1 } << " -- "
                << "0x" << std::hex << std::setfill('0') << std::setw(16)
                << ((module.ip_base + module.text.size()) - module.ip_base)
                << "]"
                << "\n";
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
                      << ": actor " << esc(2, COLOR_FG_WHITE)
                      << REPL_STATE->selected_pid->to_string()
                      << esc(2, ATTR_RESET) << " does not exist\n\r";
            return true;
        }

        auto const limit = std::stoull(std::string{ p(1).value_or("1") });

        REPL_STATE->selected_frame.reset();

        try {
            for (auto i = size_t{ 0 }; i < limit; ++i) {
                if (not proc->module.ip_in_valid_range(proc->stack.ip)) {
                    throw viua::vm::abort_execution{
                        proc->stack, "ip outside of valid range"
                    };
                }
                proc->stack.ip =
                    viua::vm::ins::execute(proc->stack, proc->stack.ip);
            }
        } catch (viua::vm::abort_execution const& e) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": aborted execution: " << e.what() << "\n\r";
            return true;
        }
    } else if (*p(0) == "stepi.g") {
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

        auto const limit = std::stoull(std::string{ p(1).value_or("1") });

        REPL_STATE->selected_frame.reset();

        try {
            for (auto i = size_t{ 0 }; i < limit; ++i) {
                auto instruction = viua::arch::instruction_type{};
                do {
                    if (not proc->module.ip_in_valid_range(proc->stack.ip)) {
                        throw viua::vm::abort_execution{
                            proc->stack, "ip outside of valid range"
                        };
                    }
                    instruction = *proc->stack.ip;
                    proc->stack.ip =
                        viua::vm::ins::execute(proc->stack, proc->stack.ip);
                } while (proc->stack.ip != nullptr);
            }
        } catch (viua::vm::abort_execution const& e) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": aborted execution: " << e.what() << "\n\r";
            return true;
        } catch (std::exception const& e) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": internal VM error: " << e.what() << "\n\r";
            if (REPL_STATE->crash_on_internal) {
                throw;
            }
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
    } else if (*p(0) == "eval" and (p(1).has_value() and *p(1) == "asm")) {
        if (not p(2).has_value()) {
            return true;
        }

        auto asm_text = std::ostringstream{};
        asm_text << *p(2);
        for (auto i = size_t{ 3 }; p(i).has_value(); ++i) {
            asm_text << ' ' << *p(i);
        }
        asm_text << '\n';
        evaluate_asm_expression(asm_text.str());
    }

    return true;
}
#endif

namespace {
auto make_breakpoint(
    Interpreter_state const& state,
    std::optional<std::string_view> const spec) -> std::optional<Breakpoint>
{
    if (not spec.has_value()) {
        if (not state.selected_pid) {
            std::println("no selected actor");
            return std::nullopt;
        }

        auto const proc = state.core->find(*state.selected_pid);
        return Breakpoint{ static_cast<size_t>(proc->stack.ip
                                               - proc->module.ip_base),
                           std::nullopt };
    } else {
        /*
         * Breakpoints with explicitly given location can be specified in
         * several ways:
         *
         *  - a raw address: 0x08
         *  - a symbol name: main
         *  - an offset into .text: main+1, foo.elf+2
         *
         * The code has to disambiguate and convert all of these forms into the
         * canonical representation ie, an offset into a .text segment.
         */

        auto const raw = std::string{ spec.value() };

        try {
            /*
             * Let's start with a raw address.
             */
            return Breakpoint{
                (viua::support::ston<size_t>(raw)
                 / sizeof(viua::arch::instruction_type)),
                std::nullopt,
            };
        } catch (std::invalid_argument const&) {
            /*
             * If the location is not convertible to a raw address, assume it is
             * a symbolic name and continue.
             */
        } catch (...) {
            /*
             * Otherwise, treat it as an error and abort.
             */
            return std::nullopt;
        }

        auto const& mod = state.core->modules.at("");

        auto fns = std::map<std::string, size_t>{};
        for (auto const& entry : mod.elf.function_table()) {
            fns.emplace(entry.second.first,
                        (entry.second.second.st_value
                         / sizeof(viua::arch::instruction_type)));
        }

        if (not fns.contains(raw)) {
            std::println("symbol not found: {}", raw);
            return std::nullopt;
        }

        return Breakpoint{
            fns.at(raw),
            raw,
        };
    }
}
}  // namespace

auto repl_eval(
    Interpreter_state& state,
    std::vector<std::string_view> const parts) -> bool
{
    auto const p = [&parts](size_t const n) -> std::optional<std::string_view>
    {
        return (n < parts.size()) ? std::optional{ parts.at(n) } : std::nullopt;
    };

    auto const leader = parts.front();
    if (leader == "quit") {
        return false;
    }

    /*
     * NEEDED COMMANDS
     * ---------------
     *
     *  - show: inspect interpreter's state
     *
     *  - info address _symbol_: "Describe where data for _symbol_ is stored"
     *  - info symbol _addr_: "Print the name of the symbol which is stored at
     *    the address _addr_"
     *  - info scope: "List all the variables local to the lexical scope", but
     *    in Viua it will print all non-void registers
     *  - info functions: list all defined functions
     *  - info variables: not implemented, but will be useful once memory
     *    tracking is ready
     *  - info main: Print name of the entry point function of the program
     *  - info modules: Print information about loaded modules
     *
     * What is the difference between "show" and "info"? The "show" commands
     * show state of the interpreter, while the "info" commands show state of
     * the program being interpreted.
     *
     *  - [b]reak: set a breakpoint
     *  - stepi: step a single instruction, optionally more
     *  - print: print value of an expression (memory location or register)
     *  - up: "select and print stack frame that called this one"
     *  - [d]own: "select and print stack frame called by this one"
     *  - [f]rame: "select and print a stack frame"
     *
     * For arguments see GDB's manual.
     */

    if (leader == "show") {
        if (not p(1)) {
            return true;
        }

        auto const subject = p(1).value();
        if (subject == "frame") {
            if (not REPL_STATE->selected_pid) {
                std::println("no selected actor");
                return true;
            }

            auto const proc = REPL_STATE->core->find(*REPL_STATE->selected_pid);
            if (not proc) {
                std::println("actor {} does not exist",
                             REPL_STATE->selected_pid->to_string());
                return true;
            }

            auto const& stack = proc->stack;
            auto const frame_index =
                p(2).transform(
                        [](std::string_view const i)
                        {
                            return viua::support::ston<size_t>(
                                std::string{ i });
                        })
                    .or_else([&state]() { return state.selected_frame; })
                    .value_or(stack.frames.size() - 1);
            if (frame_index >= stack.frames.size()) {
                std::println(
                    "selected frame index #{} is too big", frame_index);
                return true;
            }

            std::println("frame #{}", frame_index);

            auto const& frame = stack.frames.at(frame_index);

            viua::vm::backtrace::dump_registers(
                frame.parameters, stack.proc->atoms, "p");
            viua::vm::backtrace::dump_registers(
                frame.registers, stack.proc->atoms, "l");
        } else if (subject == "actor") {
            if (REPL_STATE->selected_pid) {
                std::println("actor {}", REPL_STATE->selected_pid->to_string());
            } else {
                std::println("no selected actor");
            }
        } else if (subject == "breakpoints") {
            for (auto i = size_t{ 0 }; i < state.breakpoints.size(); ++i) {
                auto const& b = state.breakpoints.at(i);
                std::println("{: 2} {}", i, b.to_string());
            }
        }
    } else if (leader == "info") {
        if (not p(1)) {
            return true;
        }

        auto const piece = p(1).value();
        if (piece == "main") {
            auto const& mod = REPL_STATE->core->modules.at("");
            if (auto const& ep = mod.elf.entry_point(); ep.has_value()) {
                std::println("{}", mod.elf.name_function_at(ep.value()));
                std::println(
                    "{} [.text+0x{:016x}]", mod.elf_path.string(), ep.value());
            } else {
                std::println("no entry point defined in main module");
            }
        } else if (piece == "functions") {
            auto const& mod = REPL_STATE->core->modules.at("");
            std::println("File {}:", mod.elf_path.string());
            for (auto const& [offset, fn_desc] : mod.elf.function_table()) {
                auto const& [fn_name, fn_sym] = fn_desc;

                auto const is_jump_label =
                    (ELF64_ST_BIND(fn_sym.st_info) == STB_LOCAL)
                    and (fn_sym.st_other == STV_HIDDEN);
                if (is_jump_label) {
                    continue;
                }

                std::println(
                    "  [.text+0x{:016x}] {}", fn_sym.st_value, fn_name);
            }
        } else if (piece == "jumps") {
            auto const& mod = REPL_STATE->core->modules.at("");
            std::println("File {}:", mod.elf_path.string());
            for (auto const& [offset, fn_desc] : mod.elf.function_table()) {
                auto const& [fn_name, fn_sym] = fn_desc;

                auto const is_jump_label =
                    (ELF64_ST_BIND(fn_sym.st_info) == STB_LOCAL)
                    and (fn_sym.st_other == STV_HIDDEN);
                if (not is_jump_label) {
                    continue;
                }

                std::println("  [.text+0x{:016x}] {}", offset, fn_name);
            }
        }
    } else if (leader == "actor") {
        if (*p(1) == "new" and p(2).has_value()) {
            if (REPL_STATE->core->modules.empty()) {
                std::println("no modules loaded");
                return true;
            }

            auto const fn_id = *p(2);
            auto const mod_name =
                std::string{ (fn_id.rfind("::") == std::string::npos)
                                 ? ""
                                 : fn_id.substr(0, fn_id.rfind("::")) };
            auto const fn_name = (fn_id.rfind("::") == std::string::npos)
                                     ? fn_id
                                     : fn_id.substr(fn_id.rfind("::") + 2);

            if (not REPL_STATE->core->modules.count(mod_name)) {
                std::println("module {} does not exist", mod_name);
                return true;
            }
            auto const& mod = REPL_STATE->core->modules.at(mod_name);

            for (auto const& each : mod.elf.function_table()) {
                if (fn_name != std::get<0>(std::get<1>(each))) {
                    continue;
                }

                auto const fn_addr = (std::get<1>(std::get<1>(each)).st_value
                                      / sizeof(viua::arch::instruction_type));
                auto pid           = REPL_STATE->core->spawn(mod_name, fn_addr);
                REPL_STATE->selected_pid =
                    std::make_unique<viua::runtime::PID>(pid);

                break;
            }
        }
    } else if (leader == "stepi") {
        if (not REPL_STATE->selected_pid) {
            std::println("no selected actor");
            return true;
        }

        auto const proc = REPL_STATE->core->find(*REPL_STATE->selected_pid);
        if (not proc) {
            std::println("actor {} does not exist",
                         REPL_STATE->selected_pid->to_string());
            return true;
        }

        auto const limit = std::stoull(std::string{ p(1).value_or("1") });

        REPL_STATE->selected_frame.reset();

        try {
            for (auto i = size_t{ 0 }; i < limit; ++i) {
                if (not proc->module.ip_in_valid_range(proc->stack.ip)) {
                    throw viua::vm::abort_execution{
                        proc->stack, "ip outside of valid range"
                    };
                }
                proc->stack.ip =
                    viua::vm::ins::execute(proc->stack, proc->stack.ip);
            }
        } catch (viua::vm::abort_execution const& e) {
            std::println("aborted execution: {}", e.what());
            return true;
        }
    } else if (leader == "run" or leader == "r") {
        if (not REPL_STATE->selected_pid) {
            std::println("no selected actor");
            return true;
        }

        auto const proc = REPL_STATE->core->find(*REPL_STATE->selected_pid);
        if (not proc) {
            std::println("actor {} does not exist",
                         REPL_STATE->selected_pid->to_string());
            return true;
        }

        REPL_STATE->selected_frame.reset();

        try {
            while (proc->module.ip_in_valid_range(proc->stack.ip)) {
                /*
                 * FIXME breakpoints
                 *
                 * This is terribly inefficient and not what a real, PROPER
                 * debugger does. A real, PROPER debugger would place a
                 * breakpoint instruction at the breakpoint address and
                 * transparently execute the intended instruction when
                 * continuing.
                 */
                {
                    auto const offset = static_cast<size_t>(
                        proc->stack.ip - proc->module.ip_base);
                    auto const& bs = state.breakpoints;
                    auto const b   = std::find_if(bs.begin(),
                                                bs.end(),
                                                [offset](auto const b)
                                                { return offset == b.offset; });

                    if (b != bs.end()) {
                        std::println("hit breakpoint #{} at {}",
                                     std::distance(b, bs.end()),
                                     b->to_string());
                        return true;
                    }
                }

                proc->stack.ip =
                    viua::vm::ins::execute(proc->stack, proc->stack.ip);
            }
            if (not proc->module.ip_in_valid_range(proc->stack.ip)) {
                throw viua::vm::abort_execution{ proc->stack,
                                                 "ip outside of valid range" };
            }
        } catch (viua::vm::abort_execution const& e) {
            std::println("aborted execution: {}", e.what());
            return true;
        }
    } else if (leader == "continue" or leader == "c") {
        if (not REPL_STATE->selected_pid) {
            std::println("no selected actor");
            return true;
        }

        auto const proc = REPL_STATE->core->find(*REPL_STATE->selected_pid);
        if (not proc) {
            std::println("actor {} does not exist",
                         REPL_STATE->selected_pid->to_string());
            return true;
        }

        REPL_STATE->selected_frame.reset();

        try {
            /*
             * Execute the first instruction ignoring any breakpoints, as long
             * as the instruction pointer is in a valid range.
             * If breakpoints were considered here it would be impossible to
             * just "continue" running the program, you would have to manually
             * step over the breaking instruction.
             */
            if (not proc->module.ip_in_valid_range(proc->stack.ip)) {
                throw viua::vm::abort_execution{ proc->stack,
                                                 "ip outside of valid range" };
            } else {
                proc->stack.ip =
                    viua::vm::ins::execute(proc->stack, proc->stack.ip);
            }

            while (proc->module.ip_in_valid_range(proc->stack.ip)) {
                /*
                 * FIXME breakpoints
                 *
                 * This is terribly inefficient and not what a real, PROPER
                 * debugger does. A real, PROPER debugger would place a
                 * breakpoint instruction at the breakpoint address and
                 * transparently execute the intended instruction when
                 * continuing.
                 */
                {
                    auto const offset = static_cast<size_t>(
                        proc->stack.ip - proc->module.ip_base);
                    auto const& bs = state.breakpoints;
                    auto const b   = std::find_if(bs.begin(),
                                                bs.end(),
                                                [offset](auto const b)
                                                { return offset == b.offset; });

                    if (b != bs.end()) {
                        std::println("hit breakpoint #{} at {}",
                                     std::distance(b, bs.end()),
                                     b->to_string());
                        return true;
                    }
                }

                proc->stack.ip =
                    viua::vm::ins::execute(proc->stack, proc->stack.ip);
            }
            if (not proc->module.ip_in_valid_range(proc->stack.ip)) {
                throw viua::vm::abort_execution{ proc->stack,
                                                 "ip outside of valid range" };
            }
        } catch (viua::vm::abort_execution const& e) {
            std::println("aborted execution: {}", e.what());
            return true;
        }
    } else if (leader == "up") {
        if (not REPL_STATE->selected_pid) {
            std::println("no selected actor");
            return true;
        }

        auto const proc = REPL_STATE->core->find(*REPL_STATE->selected_pid);
        if (not proc) {
            std::println("actor {} does not exist",
                         REPL_STATE->selected_pid->to_string());
            return true;
        }

        auto const user_frame_index =
            REPL_STATE->selected_frame.value_or(0) + 1;
        if (user_frame_index >= proc->stack.frames.size()) {
            std::println("frame {} does not exist", user_frame_index);
            return true;
        }

        REPL_STATE->selected_frame = user_frame_index;
    } else if (leader == "down") {
        if (not REPL_STATE->selected_pid) {
            std::println("no selected actor");
            return true;
        }

        auto const proc = REPL_STATE->core->find(*REPL_STATE->selected_pid);
        if (not proc) {
            std::println("actor {} does not exist",
                         REPL_STATE->selected_pid->to_string());
            return true;
        }

        auto const user_frame_index =
            REPL_STATE->selected_frame.value_or(0) - 1;
        if (user_frame_index >= proc->stack.frames.size()) {
            std::println("frame {} does not exist", user_frame_index);
            return true;
        }

        REPL_STATE->selected_frame = user_frame_index;
    } else if (leader == "frame") {
        if (not REPL_STATE->selected_pid) {
            std::println("no selected actor");
            return true;
        }

        auto const proc = REPL_STATE->core->find(*REPL_STATE->selected_pid);
        if (not proc) {
            std::println("actor {} does not exist",
                         REPL_STATE->selected_pid->to_string());
            return true;
        }

        auto const user_frame_index =
            REPL_STATE->selected_frame.value_or(0) - 1;
        if (user_frame_index >= proc->stack.frames.size()) {
            std::println("frame {} does not exist", user_frame_index);
            return true;
        }

        REPL_STATE->selected_frame = user_frame_index;
    } else if (leader == "backtrace" or leader == "bt") {
        if (not REPL_STATE->selected_pid) {
            std::println("no selected actor");
            return true;
        }

        auto const proc = REPL_STATE->core->find(*REPL_STATE->selected_pid);
        if (not proc) {
            std::println("actor {} does not exist",
                         REPL_STATE->selected_pid->to_string());
            return true;
        }

        viua::vm::backtrace::print_backtrace(proc->stack);
    } else if (leader == "break" or leader == "b") {
        auto const b = make_breakpoint(state, p(1));

        if (b.has_value()) {
            state.breakpoints.push_back(b.value());

            std::println("set breakpoint #{} at {}",
                         state.breakpoints.size() - 1,
                         state.breakpoints.back().to_string());
        } else {
            std::println("failed to set breakpoint at location {}",
                         p(1).value_or("<anonymous>"));
        }
    }

    return true;
}

auto repl_main() -> void
{
    constexpr auto DEFAULT_PROMPT = "(viua) ";

    auto raw_line = static_cast<char*>(nullptr);
    while ((raw_line = linenoise(DEFAULT_PROMPT))) {
        linenoiseHistoryAdd(raw_line);
        auto const line = std::string{ raw_line };
        free(raw_line);

        auto const useful_line =
            std::string_view{ line.empty() ? REPL_STATE->last_input : line };
        if (auto const parts = split_on_space(useful_line); not parts.empty()) {
            if (not repl_eval(*REPL_STATE, parts)) {
                break;
            }
            REPL_STATE->last_input = useful_line;
        }
    }
}

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
    auto const args = viua::libexec::args_or_exit("repl",
                                                  argc,
                                                  argv,
                                                  {
                                                      VIUA_TOOL_COMMON_OPTIONS,
                                                  });
    if (args.args.empty()) {
        std::println(stderr,
                     "{}error{}: no executable to load",
                     esc(2, COLOR_FG_RED),
                     esc(2, ATTR_RESET));
        return 1;
    }

    {
        /*
         * Enable UTF-8 support. Without this the console may act "funny" when
         * it encounters multi-byte characters.
         */
        linenoiseSetEncodingFunctions(linenoiseUtf8PrevCharLen,
                                      linenoiseUtf8NextCharLen,
                                      linenoiseUtf8ReadCode);

        /* Set the completion callback. This will be called every time the
         * user uses the <tab> key. */
        linenoiseSetCompletionCallback(completion);
        linenoiseSetHintsCallback(hints);
    }

    std::cerr << esc(1, COLOR_FG_WHITE) << "Viua REPL (debugger) "
              << esc(1, ATTR_RESET) << VIUAVM_VERSION << "\n";


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

    auto state = Interpreter_state{ io };
    REPL_STATE = viua::view_ptr{ &state };

    if (not args.args.empty()) {
        if (load_module(MAIN_MODULE_MNEMONIC, args.args.front())) {
            return 2;
        }
    }

    repl_main();

    return 0;
}
