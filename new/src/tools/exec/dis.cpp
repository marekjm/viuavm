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


#include <ctype.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <filesystem>
#include <fstream>
#include <iostream>

#include <viua/arch/ops.h>
#include <viua/libs/assembler.h>
#include <viua/support/string.h>
#include <viua/support/tty.h>
#include <viua/vm/core.h>
#include <viua/vm/elf.h>


namespace {
auto ins_to_string(viua::arch::instruction_type const ip) -> std::string
{
    auto const opcode =
        static_cast<viua::arch::opcode_type>(ip & viua::arch::ops::OPCODE_MASK);
    auto const format = static_cast<viua::arch::ops::FORMAT>(
        opcode & viua::arch::ops::FORMAT_MASK);

    switch (format) {
        using enum viua::arch::ops::FORMAT;
    case N:
        return viua::arch::ops::to_string(opcode);
    case T:
        return viua::arch::ops::T::decode(ip).to_string();
    case D:
        return viua::arch::ops::D::decode(ip).to_string();
    case S:
        return viua::arch::ops::S::decode(ip).to_string();
    case F:
        return viua::arch::ops::F::decode(ip).to_string();
    case E:
        return viua::arch::ops::E::decode(ip).to_string();
    case R:
        return viua::arch::ops::R::decode(ip).to_string();
    case M:
        return viua::arch::ops::M::decode(ip).to_string();
    default:
        return ("; " + std::string(16, '^') + " invalid instruction");
    }
}

auto match_opcode(viua::arch::instruction_type const ip,
                  viua::arch::ops::OPCODE const op,
                  viua::arch::opcode_type const flags = 0) -> bool
{
    using viua::arch::opcode_type;
    return (static_cast<opcode_type>(ip)
            == (static_cast<opcode_type>(op) | flags));
};
}  // namespace

struct Cooked_op {
    struct index_type {
        size_t physical{};  // in actual bytecode
        std::optional<size_t> physical_span;

        index_type(size_t const n) : physical{n}
        {}
    };
    index_type index;

    using op_type = std::optional<viua::arch::opcode_type>;
    op_type opcode;

    using in_type = std::optional<viua::arch::instruction_type>;
    in_type instruction;

    std::string textual_repr;

    inline auto str() const -> std::string
    {
        return textual_repr;
    }

    inline auto with_text(std::string&& s) -> Cooked_op
    {
        return Cooked_op{index, opcode, instruction, std::move(s)};
    }

    inline Cooked_op(index_type const i, op_type o, in_type n, std::string s)
            : index{i}, opcode{o}, instruction{n}, textual_repr{std::move(s)}
    {}
    inline Cooked_op(size_t const i, op_type o, in_type n, std::string s)
            : index{i}, opcode{o}, instruction{n}, textual_repr{std::move(s)}
    {}
};
using Cooked_text = std::vector<Cooked_op>;

namespace cook {
auto demangle_strtab_load(Cooked_text& raw,
                          viua::vm::elf::Fragment const& rodata,
                          viua::vm::elf::Fragment const& fntab,
                          Cooked_text& cooked,
                          size_t& i,
                          viua::arch::Register_access const out,
                          uint64_t const immediate) -> void
{
    auto const ins_at = [&raw](size_t const n) -> viua::arch::instruction_type {
        return raw.at(n).instruction.value_or(0);
    };
    auto const m = [ins_at](size_t const n,
                            viua::arch::ops::OPCODE const op,
                            viua::arch::opcode_type const flags = 0) -> bool {
        return match_opcode(ins_at(n), op, flags);
    };

    auto const read_size = [](std::vector<uint8_t> const& data,
                              size_t const off) -> uint64_t {
        auto const size_offset = (off - sizeof(uint64_t));
        auto cooked            = uint64_t{};
        memcpy(&cooked, &data[size_offset], sizeof(uint64_t));
        return le64toh(cooked);
    };

    using enum viua::arch::ops::OPCODE;
    using viua::arch::ops::D;
    using viua::arch::ops::S;
    if (m(i + 1, STRING) and S::decode(ins_at(i + 1)).out == out) {
        auto ins = raw.at(i + 1);
        auto tt  = ins.with_text(("string " + out.to_string() + ", @_strat_"
                                 + std::to_string(immediate)));
        tt.index = cooked.back().index;
        cooked.pop_back();
        tt.index.physical_span = tt.index.physical_span.value() + 1;
        cooked.emplace_back(tt);
        ++i;
        return;
    }
    if (m(i + 1, ATOM) and S::decode(ins_at(i + 1)).out == out) {
        auto ins = raw.at(i + 1);
        auto tt  = ins.with_text(("atom " + out.to_string() + ", @_strat_"
                                 + std::to_string(immediate)));
        tt.index = cooked.back().index;
        cooked.pop_back();
        tt.index.physical_span = tt.index.physical_span.value() + 1;
        cooked.emplace_back(tt);
        ++i;
        return;
    }
    if (m(i + 1, FLOAT) and S::decode(ins_at(i + 1)).out == out) {
        auto ins = raw.at(i + 1);
        cooked.pop_back();

        auto const off       = immediate;
        auto const data_size = read_size(rodata.data, off);
        auto const sv        = std::string_view{
            reinterpret_cast<char const*>(&rodata.data[off]), data_size};
        auto x = float{};
        memcpy(&x, sv.data(), sizeof(x));

        cooked.emplace_back(ins.with_text(
            ("float " + out.to_string() + ", " + std::to_string(x))));
        ++i;
        return;
    }
    if (m(i + 1, DOUBLE) and S::decode(ins_at(i + 1)).out == out) {
        auto ins = raw.at(i + 1);
        cooked.pop_back();

        auto const off       = immediate;
        auto const data_size = read_size(rodata.data, off);
        auto const sv        = std::string_view{
            reinterpret_cast<char const*>(&rodata.data[off]), data_size};
        auto x = double{};
        memcpy(&x, sv.data(), sizeof(x));

        auto ss = std::ostringstream{};
        ss << std::setprecision(std::numeric_limits<double>::digits10) << x;

        cooked.emplace_back(
            ins.with_text(("double " + out.to_string() + ", " + ss.str())));
        ++i;
        return;
    }
    if (m(i + 1, CALL) and D::decode(ins_at(i + 1)).in == out) {
        auto ins = raw.at(i + 1);

        auto const off       = immediate;
        auto const data_size = read_size(fntab.data, off);
        auto const name      = std::string{
            reinterpret_cast<char const*>(&fntab.data[off]), data_size};

        auto tt = ins.with_text(
            "call " + D::decode(ins_at(i + 1)).out.to_string() + ", " + name);
        tt.index = cooked.back().index;
        cooked.pop_back();
        tt.index.physical_span = tt.index.physical_span.value() + 1;
        cooked.emplace_back(tt);
        ++i;
        return;
    }
    if (m(i + 1, ACTOR) and D::decode(ins_at(i + 1)).in == out) {
        auto ins = raw.at(i + 1);

        auto const off       = immediate;
        auto const data_size = read_size(fntab.data, off);
        auto const name      = std::string{
            reinterpret_cast<char const*>(&fntab.data[off]), data_size};

        auto tt = ins.with_text(
            "actor " + D::decode(ins_at(i + 1)).out.to_string() + ", " + name);
        tt.index = cooked.back().index;
        cooked.pop_back();
        tt.index.physical_span = tt.index.physical_span.value() + 1;
        cooked.emplace_back(tt);
        ++i;
        return;
    }
}

auto demangle_canonical_li(Cooked_text& text,
                           viua::vm::elf::Fragment const& rodata,
                           viua::vm::elf::Fragment const& fntab) -> void
{
    auto tmp = Cooked_text{};

    auto const ins_at =
        [&text](size_t const n) -> viua::arch::instruction_type {
        return text.at(n).instruction.value_or(0);
    };
    auto const m = [ins_at](size_t const n,
                            viua::arch::ops::OPCODE const op,
                            viua::arch::opcode_type const flags = 0) -> bool {
        return match_opcode(ins_at(n), op, flags);
    };
    auto match_canonical_li = [m](size_t const n,
                                  viua::arch::ops::OPCODE const lui) -> bool {
        using enum viua::arch::ops::OPCODE;
        using viua::arch::ops::GREEDY;
        return m((n + 0), lui, GREEDY) and m((n + 1), ADDIU, GREEDY)
               and m((n + 2), ADDIU, GREEDY) and m((n + 3), MUL, GREEDY)
               and m((n + 4), ADDIU, GREEDY) and m((n + 5), ADD, GREEDY)
               and m((n + 6), ADD, GREEDY) and m((n + 7), MOVE, GREEDY)
               and (m((n + 8), MOVE) or m((n + 8), MOVE, GREEDY));
    };

    using enum viua::arch::ops::OPCODE;
    for (auto i = size_t{0}; i < text.size(); ++i) {
        if (match_canonical_li(i, LUI) or match_canonical_li(i, LUIU)) {
            using viua::arch::ops::E;
            using viua::arch::ops::R;

            auto const lui        = E::decode(ins_at(i));
            auto const high_part  = (lui.immediate << 28);
            auto const base       = R::decode(ins_at(i + 1)).immediate;
            auto const multiplier = R::decode(ins_at(i + 2)).immediate;
            auto const remainder  = R::decode(ins_at(i + 4)).immediate;

            auto const literal = high_part + (base * multiplier) + remainder;

            using viua::arch::ops::GREEDY;
            using viua::libs::assembler::li_cost;
            auto const needs_annotation =
                (li_cost(literal)
                 != li_cost(
                     std::numeric_limits<viua::arch::register_type>::max()));
            auto const needs_greedy   = m((i + 8), MOVE, GREEDY);
            auto const needs_unsigned = m(i, LUIU, GREEDY);

            auto idx = text.at(i).index;
            i += 8;
            idx.physical_span = i;
            tmp.emplace_back(
                idx,
                std::nullopt,
                std::nullopt,
                ((needs_greedy ? "g." : "") + std::string{"li "}
                 + lui.out.to_string() + ", "
                 + (needs_annotation ? "[[full]] " : "")
                 + std::to_string(literal) + (needs_unsigned ? "u" : "")));

            demangle_strtab_load(text, rodata, fntab, tmp, i, lui.out, literal);
        } else {
            tmp.push_back(std::move(text.at(i)));
        }
    }

    text = std::move(tmp);
}

auto demangle_addi_to_void(Cooked_text& text,
                           viua::vm::elf::Fragment const& rodata,
                           viua::vm::elf::Fragment const& fntab) -> void
{
    auto tmp = Cooked_text{};

    auto const ins_at =
        [&text](size_t const n) -> viua::arch::instruction_type {
        return text.at(n).instruction.value_or(0);
    };
    auto const m = [ins_at](size_t const n,
                            viua::arch::ops::OPCODE const op,
                            viua::arch::opcode_type const flags = 0) -> bool {
        return match_opcode(ins_at(n), op, flags);
    };

    using enum viua::arch::ops::OPCODE;
    for (auto i = size_t{0}; i < text.size(); ++i) {
        using viua::arch::ops::GREEDY;
        if (m(i, ADDI) or m(i, ADDIU) or m(i, ADDI, GREEDY)
            or m(i, ADDIU, GREEDY)) {
            using viua::arch::ops::R;
            using viua::arch::ops::S;
            auto const addi = R::decode(ins_at(i));
            if (addi.in.is_void()) {
                auto const needs_greedy = (addi.opcode & GREEDY);
                auto const needs_unsigned =
                    (m(i, ADDIU, GREEDY) or m(i, ADDIU));

                auto idx          = text.at(i).index;
                idx.physical_span = idx.physical;
                tmp.emplace_back(idx,
                                 std::nullopt,
                                 std::nullopt,
                                 ((needs_greedy ? "g." : "")
                                  + std::string{"li "} + addi.out.to_string()
                                  + ", " + std::to_string(addi.immediate)
                                  + (needs_unsigned ? "u" : "")));

                demangle_strtab_load(
                    text, rodata, fntab, tmp, i, addi.out, addi.immediate);

                continue;
            }
        }

        tmp.push_back(std::move(text.at(i)));
    }

    text = std::move(tmp);
}

auto demangle_addiu(Cooked_text& text) -> void
{
    auto tmp = Cooked_text{};

    auto const ins_at =
        [&text](size_t const n) -> viua::arch::instruction_type {
        return text.at(n).instruction.value_or(0);
    };
    auto const m = [ins_at](size_t const n,
                            viua::arch::ops::OPCODE const op,
                            viua::arch::opcode_type const flags = 0) -> bool {
        return match_opcode(ins_at(n), op, flags);
    };

    using enum viua::arch::ops::OPCODE;
    for (auto i = size_t{0}; i < text.size(); ++i) {
        using viua::arch::ops::GREEDY;
        if (m(i, ADDIU) or m(i, ADDIU, GREEDY)) {
            using viua::arch::ops::R;
            using viua::arch::ops::S;
            auto const addi         = R::decode(ins_at(i));
            auto const needs_greedy = (addi.opcode & GREEDY);

            auto idx          = text.at(i).index;
            idx.physical_span = idx.physical;
            tmp.emplace_back(
                idx,
                std::nullopt,
                std::nullopt,
                ((needs_greedy ? "g." : "") + std::string{"addi "}
                 + addi.out.to_string() + ", " + addi.in.to_string() + ", "
                 + std::to_string(addi.immediate) + 'u'));
            continue;
        }

        tmp.push_back(std::move(text.at(i)));
    }

    text = std::move(tmp);
}

auto demangle_memory(Cooked_text& text) -> void
{
    auto tmp = Cooked_text{};

    auto const ins_at =
        [&text](size_t const n) -> viua::arch::instruction_type {
        return text.at(n).instruction.value_or(0);
    };
    auto const m = [ins_at](size_t const n,
                            viua::arch::ops::OPCODE const op,
                            viua::arch::opcode_type const flags = 0) -> bool {
        return match_opcode(ins_at(n), op, flags);
    };

    using enum viua::arch::ops::OPCODE;
    for (auto i = size_t{0}; i < text.size(); ++i) {
        using viua::arch::ops::GREEDY;
        if (m(i, SM) or m(i, LM) or m(i, AA) or m(i, AD)) {
            using viua::arch::ops::M;
            auto const raw_op = ins_at(i);
            auto const op     = M::decode(raw_op);

            auto name = std::string{};
            switch (static_cast<viua::arch::ops::OPCODE>(op.opcode)) {
            case SM:
                name = "s";
                break;
            case LM:
                name = "l";
                break;
            case AA:
            case AD:
                name = "am";
                break;
            default:
                abort();
            }
            switch (op.spec) {
            case 0:
                name += "b";
                break;
            case 1:
                name += "h";
                break;
            case 2:
                name += "w";
                break;
            case 3:
                name += "d";
                break;
            case 4:
                name += "q";
                break;
            default:
                abort();
                break;
            }
            switch (static_cast<viua::arch::ops::OPCODE>(op.opcode)) {
            case AA:
                name += "a";
                break;
            case AD:
                name += "d";
                break;
            default:
                break;
            }

            auto idx = text.at(i).index;
            tmp.emplace_back(
                idx,
                op.opcode,
                raw_op,
                (name + " " + op.out.to_string() + ", " + op.in.to_string()
                 + ", " + std::to_string(op.immediate)));
            continue;
        }
        if (m(i, CAST)) {
            using viua::arch::ops::E;
            auto const raw_op = ins_at(i);
            auto const op     = E::decode(raw_op);

            auto desired_type = std::string{"void"};
            switch (static_cast<viua::vm::Register::Types>(op.immediate)) {
                using enum viua::vm::Register::Types;
            case INT:
                desired_type = "int";
                break;
            case UINT:
                desired_type = "uint";
                break;
            case FLOAT32:
                desired_type = "float";
                break;
            case FLOAT64:
                desired_type = "double";
                break;
            case POINTER:
                desired_type = "pointer";
                break;
            case ATOM:
                desired_type = "atom";
                break;
            case PID:
                desired_type = "pid";
                break;
            case VOID:
            case UNDEFINED:
            default:
                desired_type = "<invalid>";
                break;
            }

            auto idx = text.at(i).index;
            tmp.emplace_back(
                idx,
                op.opcode,
                raw_op,
                ("cast " + op.out.to_string() + ", " + desired_type));
            continue;
        }

        tmp.push_back(std::move(text.at(i)));
    }

    text = std::move(tmp);
}

auto demangle_branches(Cooked_text& raw) -> std::map<size_t, size_t>
{
    auto const ins_at = [&raw](size_t const n) -> viua::arch::instruction_type {
        return raw.at(n).instruction.value_or(0);
    };
    auto const m = [ins_at](size_t const n,
                            viua::arch::ops::OPCODE const op,
                            viua::arch::opcode_type const flags = 0) -> bool {
        return match_opcode(ins_at(n), op, flags);
    };

    auto cooked              = Cooked_text{};
    auto physical_to_logical = std::map<size_t, size_t>{};
    {
        auto drift = size_t{0};
        for (auto i = size_t{0}; i < raw.size(); ++i) {
            using enum viua::arch::ops::OPCODE;

            if (m(i, IF)) {
                ++drift;
            }

            physical_to_logical[raw.at(i).index.physical] = (i - drift);
        }
    }

    for (auto i = size_t{0}; i < raw.size(); ++i) {
        using enum viua::arch::ops::OPCODE;
        auto const s = raw.at(i).str();
        if (s.starts_with("g.li") and m(i + 1, IF)) {
            auto const phys_index    = std::stoull(s.substr(s.rfind(' ')));
            auto const logical_index = physical_to_logical.at(phys_index);

            auto branch                = std::move(raw.at(++i));
            branch.index.physical      = raw.at(i - 1).index.physical;
            branch.index.physical_span = branch.index.physical;
            branch.textual_repr =
                branch.textual_repr.substr(0, branch.textual_repr.rfind(' '))
                + ' ' + std::to_string(logical_index);

            cooked.push_back(branch);

            continue;
        }

        cooked.push_back(std::move(raw.at(i)));
    }

    raw = std::move(cooked);

    return physical_to_logical;
}
}  // namespace cook

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
                  << ": no file to disassemble\n";
        return 1;
    }

    auto preferred_output_path = std::optional<std::filesystem::path>{};
    auto demangle_li           = true;
    auto verbosity_level       = 0;
    auto show_version          = false;
    auto show_help             = false;
    auto singles               = std::vector<viua::arch::instruction_type>{};

    for (auto i = decltype(args)::size_type{}; i < args.size(); ++i) {
        auto const& each = args.at(i);
        if (each == "--") {
            // explicit separator of options and operands
            break;
        }
        /*
         * Tool-specific options.
         */
        else if (each == "--no-demangle=all") {
            demangle_li = false;
        } else if (each == "--no-demangle=li") {
            demangle_li = false;
        } else if (each == "-i") {
            singles.push_back(std::stoull(args.at(++i), nullptr, 0));
        } else if (each == "-o") {
            preferred_output_path = std::filesystem::path{args.at(++i)};
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
        if (execlp("man", "man", "1", "viua-dis", nullptr) == -1) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": man(1) page not installed or not found\n";
            return 1;
        }
    }

    if (singles.size()) {
        std::cout << std::hex << std::setfill('0');
        for (auto const each : singles) {
            std::cout << "0x" << std::setw(16) << each << "  "
                      << ins_to_string(each) << "\n";
        }
        return 0;
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
        struct stat statbuf {};
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

    auto text = std::vector<viua::arch::instruction_type>{};
    if (auto const f = main_module.find_fragment(".text"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": no text fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .text section found\n";
        return 1;
    } else {
        text = main_module.make_text_from(f->get().data);
    }

    auto entry_addr = size_t{0};
    if (auto const ep = main_module.entry_point(); ep.has_value()) {
        entry_addr = *ep;
    } else {
        std::cerr << esc(2, COLOR_FG_WHITE) << elf_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": no entry point defined\n";
        return 1;
    }

    auto to_file = std::ofstream{};
    if (preferred_output_path.has_value()) {
        to_file.open(*preferred_output_path);
    }
    auto& out = (preferred_output_path.has_value() ? to_file : std::cout);

    auto const rodata = main_module.find_fragment(".rodata");
    if (rodata.has_value()) {
        auto const& strtab = rodata->get();
        auto dumped        = false;

        for (auto off = size_t{8}; off < strtab.data.size();
             off += sizeof(uint64_t)) {
            auto const data_size = [&strtab, off]() -> uint64_t {
                auto const size_offset = (off - sizeof(uint64_t));
                auto tmp               = uint64_t{};
                memcpy(&tmp, &strtab.data[size_offset], sizeof(uint64_t));
                return le64toh(tmp);
            }();
            auto const sv = std::string_view{
                reinterpret_cast<char const*>(&strtab.data[off]), data_size};
            auto const is_string =
                std::all_of(sv.begin(), sv.end(), [](char const c) -> bool {
                    return (::isprint(c) or ::isspace(c));
                });

            if (not is_string) {
                /*
                 * Do not dump non-string data. These are floats and doubles and
                 * they will be merged with their pseudoinstructions during
                 * demangling.
                 */
                off += sv.size();
                continue;
            }

            dumped = true;
            out << "; [.rodata+0x" << std::hex << std::setw(16)
                << std::setfill('0') << off << "] to"
                << " [.rodata+0x" << std::hex << std::setw(16)
                << std::setfill('0') << (off + data_size) << "] (" << std::dec
                << data_size << " byte" << (data_size == 1 ? "" : "s") << ")\n";
            out << ".label: _strat_" << off << "\n";

            out << ".value: string " << viua::support::string::quoted(sv)
                << "\n\n";

            off += sv.size();
        }

        if (dumped) {
            out << "\n";
        }
    }

    auto const ft    = main_module.function_table();
    auto ordered_fns = std::vector<std::tuple<std::string, size_t, size_t>>{};
    {
        for (auto const& each : ft) {
            ordered_fns.push_back({each.second.first, each.second.second, 0});
        }
        std::sort(ordered_fns.begin(),
                  ordered_fns.end(),
                  [](auto const& a, auto const& b) {
                      return (std::get<1>(a) < std::get<1>(b));
                  });
        for (auto i = size_t{1}; i < ordered_fns.size(); ++i) {
            auto const& each  = ordered_fns.at(i);
            auto& prev        = ordered_fns.at(i - 1);
            std::get<2>(prev) = (std::get<1>(each) - std::get<1>(prev))
                                / sizeof(viua::arch::instruction_type);
        }
        std::get<2>(ordered_fns.back()) =
            ((text.size() * sizeof(viua::arch::instruction_type))
             - std::get<1>(ordered_fns.back()))
            / sizeof(viua::arch::instruction_type);
    }

    auto ef = main_module.name_function_at(entry_addr);
    for (auto const& [name, addr, size] : ordered_fns) {
        /*
         * Let's emit a comment containing the span of the function. This is
         * would be useful if you wanted to map the disassembled span to bytes
         * inside the bytecode segment.
         */
        out << "; [.text+0x" << std::setw(16) << std::setfill('0') << addr
            << "] to "
            << "[.text+0x" << std::setw(16) << std::setfill('0')
            << (addr + (size * sizeof(viua::arch::instruction_type))) << "] ("
            << size << " instruction" << ((size > 1) ? "s" : "") << ")\n";

        /*
         * Then, the name. Marking the entry point is necessary to correctly
         * recreate the behaviour of the program.
         */
        out << ".function: " << ((ef.first == name) ? "[[entry_point]] " : "")
            << name << "\n";

        auto cooked_text  = Cooked_text{};
        auto const offset = (addr / sizeof(viua::arch::instruction_type));
        for (auto i = size_t{0}; i < size; ++i) {
            auto const ip     = text.at(offset + i);
            auto const opcode = static_cast<viua::arch::opcode_type>(
                ip & viua::arch::ops::OPCODE_MASK);
            cooked_text.emplace_back(i, opcode, ip, ins_to_string(ip));
        }

        if (demangle_li) {
            auto const fntab = main_module.find_fragment(".viua.fns");
            cook::demangle_canonical_li(
                cooked_text, rodata->get(), fntab->get());
            cook::demangle_addi_to_void(
                cooked_text, rodata->get(), fntab->get());
        }
        cook::demangle_addiu(cooked_text);
        cook::demangle_memory(cooked_text);

        auto const physical_to_logical = cook::demangle_branches(cooked_text);

        out << "    ; <binary>          <ip>               "
               "<logical>:<physical-span>\n";
        for (auto i = size_t{}; i < cooked_text.size(); ++i) {
            auto const& [index, op, ip, s] = cooked_text.at(i);
            out << "    ; ";
            if (ip.has_value()) {
                out << std::setw(16) << std::setfill('0') << std::hex << *ip;
            } else {
                out << std::string(16, ' ');
            }

            out << "  ";
            out << std::setw(16) << std::setfill('0') << std::hex
                << (addr
                    + (sizeof(viua::arch::instruction_type) * index.physical));

            out << "  " << std::dec << std::setw(2) << std::setfill(' ') << i
                << ":" << index.physical;
            if (index.physical_span.has_value()) {
                out << "-" << *index.physical_span;
            }
            out << "\n";

            out << "    " << s << "\n";
        }

        out << ".end\n\n";
    }

    return 0;
}
