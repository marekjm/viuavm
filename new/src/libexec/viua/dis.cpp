/*
 *  Copyright (C) 2022-2023 Marek Marecki
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
#include <endian.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <regex>

#include <viua/arch/ops.h>
#include <viua/libexec/common.hh>
#include <viua/libs/assembler.h>
#include <viua/libs/lexer.h>
#include <viua/support/errno.h>
#include <viua/support/memory.h>
#include <viua/support/print.hh>
#include <viua/support/string.h>
#include <viua/support/tty.h>
#include <viua/vm/core.h>
#include <viua/vm/elf.h>

namespace {
auto ins_to_string(
    viua::arch::instruction_type const ip) -> std::string
{
    auto const opcode = viua::carve_opcode_out(ip);
    auto const format = viua::carve_format_out(opcode);

    std::println("got format: {:04x} ({})",
                 static_cast<uint16_t>(format),
                 to_string(format));

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
        case I:
            return viua::arch::ops::I::decode(ip).to_string();
        case U:
            return viua::arch::ops::U::decode(ip).to_string();
        case M:
            return viua::arch::ops::M::decode(ip).to_string();
        default:
            return ("; " + std::string(16, '^') + " invalid instruction");
    }
}

auto match_opcode(
    viua::arch::instruction_type const ip,
    viua::arch::ops::OPCODE const op,
    viua::arch::opcode_type const flags = 0) -> bool
{
    auto const opcode = viua::carve_opcode_out(ip);
    using viua::arch::opcode_type;
    return (opcode == (static_cast<opcode_type>(op) | flags));
}

auto main_module_elf_type = ET_NONE;
auto get_symbol_name_in_executable(
    uint64_t const value,
    std::vector<Elf64_Sym> const& symtab,
    viua::vm::elf::Strtab_view const strtab) -> std::string
{
    auto sym = std::find_if(symtab.begin(),
                            symtab.end(),
                            [value](Elf64_Sym const each) -> bool
                            { return (each.st_value == value); });
    if (sym == symtab.end()) {
        abort();  // FIXME std::optional?
    }
    return std::string{ strtab.view_at(sym->st_name) };
}
auto get_symbol_name_in_relocatable(
    uint64_t const value,
    std::vector<Elf64_Sym> const& symtab,
    viua::vm::elf::Strtab_view const strtab) -> std::string
{
    return std::string{ strtab.view_at(symtab.at(value).st_name) };
}
auto get_symbol_name(
    uint64_t const value,
    std::vector<Elf64_Sym> const& symtab,
    viua::vm::elf::Strtab_view const strtab) -> std::string
{
    switch (main_module_elf_type) {
        case ET_EXEC:
            return get_symbol_name_in_executable(value, symtab, strtab);
        case ET_REL:
            return get_symbol_name_in_relocatable(value, symtab, strtab);
        default:
            abort();  // FIXME std::optional?
    }
}

auto match_atom(
    std::string_view sv) -> bool
{
    std::regex re{ viua::libs::lexer::pattern::LITERAL_ATOM };
    std::cmatch m;
    return std::regex_match(sv.data(), m, re);
}

auto is_jump_label(
    Elf64_Sym const sym) -> bool
{
    if (ELF64_ST_TYPE(sym.st_info) != STT_FUNC) {
        return false;
    }
    if (ELF64_ST_BIND(sym.st_info) != STB_LOCAL) {
        return false;
    }
    if (sym.st_other != STV_HIDDEN) {
        return false;
    }
    return true;
}
auto is_extern(
    Elf64_Sym const sym) -> bool
{
    return sym.st_value == 0;
}
}  // namespace

struct Cooked_op {
    struct index_type {
        size_t physical{};  // in actual bytecode
        std::optional<size_t> physical_span;

        index_type(
            size_t const n)
            : physical{ n }
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

    inline auto with_text(
        std::string&& s) -> Cooked_op
    {
        return Cooked_op{ index, opcode, instruction, std::move(s) };
    }

    inline Cooked_op(
        index_type const i,
        op_type o,
        in_type n,
        std::string s)
        : index{ i }
        , opcode{ o }
        , instruction{ n }
        , textual_repr{ std::move(s) }
    {}
    inline Cooked_op(
        size_t const i,
        op_type o,
        in_type n,
        std::string s)
        : index{ i }
        , opcode{ o }
        , instruction{ n }
        , textual_repr{ std::move(s) }
    {}
};
using Cooked_text = std::vector<Cooked_op>;

namespace cook {
namespace {
auto make_label_ref(
    viua::vm::elf::Strtab_view const strtab,
    Elf64_Sym const& sym) -> std::string
{
    return ("@" + std::string{ strtab.view_at(sym.st_name) });
}
auto read_size(
    std::vector<uint8_t> const& data,
    size_t const off) -> uint64_t
{
    auto const size_offset = (off - sizeof(uint64_t));
    return le64toh(viua::support::memload<uint64_t>(&data[size_offset]));
}
auto load_string(
    std::vector<uint8_t> const& data,
    size_t const off) -> std::string
{
    auto s = std::string{ reinterpret_cast<char const*>(data.data() + off),
                          read_size(data, off) };
    auto const needs_quotes =
        std::any_of(s.begin(),
                    s.end(),
                    [](auto const each) -> bool
                    { return not(std::isalnum(each) or (each == '_')); });
    if (needs_quotes) {
        s = ('"' + s + '"');
    }
    return s;
}
auto view_data(
    std::vector<uint8_t> const& data,
    size_t const off) -> std::string_view
{
    return std::string_view{ reinterpret_cast<char const*>(data.data() + off),
                             read_size(data, off) };
}
}  // namespace

auto demangle_symbol_load(
    Cooked_text& raw,
    Cooked_text& cooked,
    size_t& i,
    viua::arch::Register_access const out,
    uint64_t const immediate,
    std::vector<Elf64_Sym> const& symtab,
    viua::vm::elf::Strtab_view const strtab,
    std::vector<uint8_t> const& rodata) -> void
{
    auto const ins_at = [&raw](size_t const n) -> viua::arch::instruction_type
    { return raw.at(n).instruction.value_or(0); };
    auto const m = [ins_at](size_t const n,
                            viua::arch::ops::OPCODE const op,
                            viua::arch::opcode_type const flags = 0) -> bool
    { return match_opcode(ins_at(n), op, flags); };

    using enum viua::arch::ops::OPCODE;
    using viua::arch::ops::D;
    using viua::arch::ops::S;
    if (m(i + 1, ATOM) and S::decode(ins_at(i + 1)).out == out) {
        auto ins = raw.at(i + 1);

        auto const sym_it = std::find_if(
            symtab.begin(),
            symtab.end(),
            [immediate](auto const& each) -> bool
            {
                return (each.st_value == immediate)
                       and (ELF64_ST_TYPE(each.st_info) == STT_OBJECT);
            });
        /*
         * FIXME OvsELF
         * In executable files we need to find by .st_value ie, the real offset.
         * In relocatable files we need to find by index into symbol table since
         * relocations have not been performed yet.
         */
        auto const& sym =
            (sym_it == symtab.end()) ? symtab.at(immediate) : *sym_it;
        auto const label_or_value = sym.st_name
                                        ? make_label_ref(strtab, sym)
                                        : load_string(rodata, immediate);

        auto tt =
            ins.with_text("atom " + out.to_string() + ", " + label_or_value);
        tt.index = cooked.back().index;
        cooked.pop_back();
        tt.index.physical_span = tt.index.physical_span.value() + 1;
        cooked.emplace_back(tt);
        ++i;
        return;
    }
    if (m(i + 1, DOUBLE) and S::decode(ins_at(i + 1)).out == out) {
        auto ins = raw.at(i + 1);
        cooked.pop_back();

        auto const sv = view_data(rodata, immediate);
        auto x        = double{};
        memcpy(&x, sv.data(), sizeof(x));

        auto ss = std::ostringstream{};
        ss << std::fixed
           << std::setprecision(std::numeric_limits<double>::digits10) << x;

        cooked.emplace_back(
            ins.with_text(("double " + out.to_string() + ", " + ss.str())));
        ++i;
        return;
    }
    if (m(i + 1, CALL) and D::decode(ins_at(i + 1)).in == out) {
        auto ins = raw.at(i + 1);

        auto const sym_name = get_symbol_name(immediate, symtab, strtab);
        auto const safe_sym_name =
            match_atom(sym_name) ? sym_name : ('"' + sym_name + '"');

        auto tt =
            ins.with_text("call " + D::decode(ins_at(i + 1)).out.to_string()
                          + ", " + safe_sym_name);
        tt.index = cooked.back().index;
        cooked.pop_back();
        tt.index.physical_span = tt.index.physical_span.value() + 1;
        cooked.emplace_back(tt);
        ++i;
        return;
    }
    if (m(i + 1, ACTOR) and D::decode(ins_at(i + 1)).in == out) {
        auto ins = raw.at(i + 1);

        auto const sym_name = get_symbol_name(immediate, symtab, strtab);
        auto const safe_sym_name =
            match_atom(sym_name) ? sym_name : ('"' + sym_name + '"');

        auto tt =
            ins.with_text("actor " + D::decode(ins_at(i + 1)).out.to_string()
                          + ", " + safe_sym_name);
        tt.index = cooked.back().index;
        cooked.pop_back();
        tt.index.physical_span = tt.index.physical_span.value() + 1;
        cooked.emplace_back(tt);
        ++i;
        return;
    }
    if (m(i + 1, IF) and D::decode(ins_at(i + 1)).in == out) {
        auto ins = raw.at(i + 1);

        auto const sym_name = get_symbol_name(immediate, symtab, strtab);
        auto const safe_sym_name =
            match_atom(sym_name) ? sym_name : ('"' + sym_name + '"');

        auto tt = ins.with_text("if " + D::decode(ins_at(i + 1)).out.to_string()
                                + ", " + safe_sym_name);
        tt.index = cooked.back().index;
        cooked.pop_back();
        tt.index.physical_span = tt.index.physical_span.value() + 1;
        cooked.emplace_back(tt);
        ++i;
        return;
    }
}

auto demangle_canonical_li(
    Cooked_text& text,
    std::vector<Elf64_Sym> const& symtab,
    viua::vm::elf::Strtab_view const strtab,
    std::vector<uint8_t> const& rodata) -> void
{
    auto tmp = Cooked_text{};

    auto const ins_at = [&text](size_t const n) -> viua::arch::instruction_type
    { return text.at(n).instruction.value_or(0); };
    auto const m = [ins_at](size_t const n,
                            viua::arch::ops::OPCODE const op,
                            viua::arch::opcode_type const flags = 0) -> bool
    { return match_opcode(ins_at(n), op, flags); };
    auto match_canonical_li = [m](size_t const n,
                                  viua::arch::ops::OPCODE const lui) -> bool
    {
        using enum viua::arch::ops::OPCODE;

        auto const li = m((n + 0), lui);
        return li and (m((n + 1), ADDI) or m((n + 1), ADDIU));
    };

    using enum viua::arch::ops::OPCODE;
    for (auto i = size_t{ 0 }; i < text.size(); ++i) {
        if (match_canonical_li(i, LUI) or match_canonical_li(i, LUIU)) {
            using viua::arch::ops::I;
            using viua::arch::ops::U;

            auto const luiu = I::decode(ins_at(i));
            auto const high_part =
                (static_cast<uint64_t>(luiu.immediate) << 32);
            auto const addiu    = U::decode(ins_at(i + 1));
            auto const low_part = addiu.immediate;

            auto const value = (high_part | low_part);

            auto const needs_unsigned = m(i, LUIU);

            auto const literal =
                needs_unsigned
                    ? ((value == std::numeric_limits<uint64_t>::max())
                           ? "-1u"
                           : (std::to_string(value) + 'u'))
                    : std::to_string(static_cast<int64_t>(value));

            auto idx = text.at(i).index;
            ++i;  // skip the LLI
            idx.physical_span = i;

            // FIXME detect if adding [[full]] is really needed
            tmp.emplace_back(
                idx,
                std::nullopt,
                std::nullopt,
                std::format(
                    "[[full]] li {}, {}", luiu.out.to_string(), literal));

            // FIXME calls are using ATXTP instead of LUIU
            if (needs_unsigned) {
                demangle_symbol_load(
                    text, tmp, i, luiu.out, value, symtab, strtab, rodata);
            }
        } else {
            tmp.push_back(std::move(text.at(i)));
        }
    }

    text = std::move(tmp);
}

auto demangle_short_li(
    Cooked_text& text) -> void
{
    auto tmp = Cooked_text{};

    auto const ins_at = [&text](size_t const n) -> viua::arch::instruction_type
    { return text.at(n).instruction.value_or(0); };
    auto const m = [ins_at](size_t const n,
                            viua::arch::ops::OPCODE const op,
                            viua::arch::opcode_type const flags = 0) -> bool
    { return match_opcode(ins_at(n), op, flags); };

    using enum viua::arch::ops::OPCODE;
    for (auto i = size_t{ 0 }; i < text.size(); ++i) {
        // FIXME Only match ADDI and check for UNSIGNED flag later.
        if (m(i, ADDI) or m(i, ADDIU)) {
            using viua::arch::ops::S;
            using viua::arch::ops::U;
            auto const addi = U::decode(ins_at(i));
            if (addi.in.is_void()) {
                // FIXME Check for UNSIGNED FLAG, not for the instruction.
                auto const needs_unsigned = m(i, ADDIU);

                auto const value = addi.immediate;
                auto const literal =
                    needs_unsigned
                        ? (std::to_string(value) + 'u')
                        : std::to_string(static_cast<int32_t>(value));

                auto idx          = text.at(i).index;
                idx.physical_span = idx.physical;
                tmp.emplace_back(
                    idx,
                    std::nullopt,
                    std::nullopt,
                    std::format("li {}, {}", addi.out.to_string(), literal));

                continue;
            }
        }

        tmp.push_back(std::move(text.at(i)));
    }

    text = std::move(tmp);
}

auto demangle_addiu(
    Cooked_text& text) -> void
{
    auto tmp = Cooked_text{};

    auto const ins_at = [&text](size_t const n) -> viua::arch::instruction_type
    { return text.at(n).instruction.value_or(0); };
    auto const m = [ins_at](size_t const n,
                            viua::arch::ops::OPCODE const op,
                            viua::arch::opcode_type const flags = 0) -> bool
    { return match_opcode(ins_at(n), op, flags); };

    using enum viua::arch::ops::OPCODE;
    for (auto i = size_t{ 0 }; i < text.size(); ++i) {
        if (m(i, ADDIU) or m(i, ADDIU)) {
            using viua::arch::ops::S;
            using viua::arch::ops::U;
            auto const addi = U::decode(ins_at(i));

            auto idx          = text.at(i).index;
            idx.physical_span = idx.physical;
            tmp.emplace_back(idx,
                             std::nullopt,
                             std::nullopt,
                             std::format("addi {}, {}, {}u",
                                         addi.out.to_string(),
                                         addi.in.to_string(),
                                         std::to_string(addi.immediate)));
            continue;
        }

        tmp.push_back(std::move(text.at(i)));
    }

    text = std::move(tmp);
}

auto demangle_arodp(
    Cooked_text& text,
    std::vector<Elf64_Sym> const& symtab,
    viua::vm::elf::Strtab_view const& strtab,
    std::vector<uint8_t> const& rodata) -> void
{
    auto tmp          = Cooked_text{};
    auto const ins_at = [&text](size_t const n) -> viua::arch::instruction_type
    { return text.at(n).instruction.value_or(0); };
    auto const m = [ins_at](size_t const n,
                            viua::arch::ops::OPCODE const op,
                            viua::arch::opcode_type const flags = 0) -> bool
    { return match_opcode(ins_at(n), op, flags); };

    using enum viua::arch::ops::OPCODE;
    for (auto i = size_t{ 0 }; i < text.size(); ++i) {
        if (m(i, ARODP)) {
            using viua::arch::ops::I;
            auto const arodp = I::decode(ins_at(i));

            auto const off = arodp.immediate;

            auto const sym = std::find_if(
                symtab.begin(),
                symtab.end(),
                [off](auto const& each) -> bool
                {
                    return (each.st_value == off)
                           and (ELF64_ST_TYPE(each.st_info) == STT_OBJECT);
                });
            auto const label_or_value = (sym == symtab.end())
                                            ? load_string(rodata, off)
                                            : make_label_ref(strtab, *sym);

            auto idx          = text.at(i).index;
            idx.physical_span = idx.physical;
            tmp.emplace_back(
                idx,
                std::nullopt,
                std::nullopt,
                std::format(
                    "arodp {}, {}", arodp.out.to_string(), label_or_value));
            continue;
        }
        if (m(i, ATXTP)) {
            using viua::arch::ops::I;
            auto const atxtp = I::decode(ins_at(i));

            auto const off = atxtp.immediate;

            auto const sym_it = std::find_if(
                symtab.begin(),
                symtab.end(),
                [off](auto const& each) -> bool
                {
                    return (each.st_value == off)
                           and (ELF64_ST_TYPE(each.st_info) == STT_FUNC);
                });
            /*
             * FIXME OvsELF
             * In executable files we need to find by .st_value ie, the real
             * offset. In relocatable files we need to find by index into symbol
             * table since relocations have not been performed yet.
             */
            auto const& sym =
                (sym_it == symtab.end()) ? symtab.at(off) : *sym_it;

            auto idx          = text.at(i).index;
            idx.physical_span = idx.physical;
            tmp.emplace_back(idx,
                             std::nullopt,
                             std::nullopt,
                             std::format("atxtp {}, {}",
                                         atxtp.out.to_string(),
                                         make_label_ref(strtab, sym)));

            demangle_symbol_load(text,
                                 tmp,
                                 i,
                                 atxtp.out,
                                 atxtp.immediate,
                                 symtab,
                                 strtab,
                                 rodata);

            continue;
        }

        tmp.push_back(std::move(text.at(i)));
    }

    text = std::move(tmp);
}

auto demangle_memory(
    Cooked_text& text) -> void
{
    auto tmp = Cooked_text{};

    auto const ins_at = [&text](size_t const n) -> viua::arch::instruction_type
    { return text.at(n).instruction.value_or(0); };
    auto const m = [ins_at](size_t const n,
                            viua::arch::ops::OPCODE const op,
                            viua::arch::opcode_type const flags = 0) -> bool
    { return match_opcode(ins_at(n), op, flags); };

    using enum viua::arch::ops::OPCODE;
    for (auto i = size_t{ 0 }; i < text.size(); ++i) {
        auto const memory_op = (m(i, SM) or m(i, LM) or m(i, AA) or m(i, AD));
        if (memory_op) {
            using viua::arch::ops::M;
            auto const raw_op = ins_at(i);
            auto const op     = M::decode(raw_op);

            auto name = std::string{};
            switch (static_cast<viua::arch::ops::OPCODE>(op.opcode)) {
                case SM:
                    name += "s";
                    break;
                case LM:
                    name += "l";
                    break;
                case AA:
                case AD:
                    name += "am";
                    break;
                default:
                    abort();
            }
            switch (op.get_spec()) {
                using namespace viua::arch::ops::OPCODE_FLAGS;
                case UNIT_BYTE:
                    name += "b";
                    break;
                case UNIT_HALF_WORD:
                    name += "h";
                    break;
                case UNIT_WORD:
                    name += "w";
                    break;
                case UNIT_DOUBLE_WORD:
                    name += "d";
                    break;
                case UNIT_QUAD_WORD:
                    name += "q";
                    break;
                case UNIT_OCTA_WORD:
                    name += "o";
                    break;
                case UNIT_HEXA_WORD:
                    name += "x";
                    break;
                case UNIT_DUOTRI_WORD:
                    name += "u";
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
                 + ", "
                 + std::to_string(static_cast<uintmax_t>(op.immediate))));
            continue;
        }
        if (m(i, CAST)) {
            using viua::arch::ops::I;
            auto const raw_op = ins_at(i);
            auto const op     = I::decode(raw_op);

            auto desired_type = std::string{ "void" };
            switch (static_cast<viua::arch::FUNDAMENTAL_TYPES>(op.immediate)) {
                using enum viua::arch::FUNDAMENTAL_TYPES;
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

auto demangle_branches(
    Cooked_text& raw) -> std::map<size_t, size_t>
{
    auto const ins_at = [&raw](size_t const n) -> viua::arch::instruction_type
    { return raw.at(n).instruction.value_or(0); };
    auto const m = [ins_at](size_t const n,
                            viua::arch::ops::OPCODE const op,
                            viua::arch::opcode_type const flags = 0) -> bool
    { return match_opcode(ins_at(n), op, flags); };

    auto cooked              = Cooked_text{};
    auto physical_to_logical = std::map<size_t, size_t>{};
    {
        auto drift = size_t{ 0 };
        for (auto i = size_t{ 0 }; i < raw.size(); ++i) {
            using enum viua::arch::ops::OPCODE;

            if (m(i, IF)) {
                ++drift;
            }

            physical_to_logical[raw.at(i).index.physical] = (i - drift);
        }
    }

    for (auto i = size_t{ 0 }; i < raw.size(); ++i) {
        using enum viua::arch::ops::OPCODE;
        auto const s = raw.at(i).str();
        if (s.starts_with("li") and m(i + 1, IF)) {
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
    auto const args = viua::libexec::args_or_exit(
        "dis",
        argc,
        argv,
        {
            VIUA_TOOL_COMMON_OPTIONS,
            { { "o", { "out" } }, Args::Kind::Single },
            { { "i", { "instruction" } }, Args::Kind::List },
            { { "N", { "no-demangle" } }, Args::Kind::Set },
        });
    auto const singles =
        args.get<std::vector<std::string_view>>("instruction")
            .transform(
                [](auto value)
                {
                    auto singles = std::vector<uint64_t>{};
                    singles.reserve(value.size());
                    for (auto const& i : value) {
                        singles.push_back(std::strtoull(i.data(), nullptr, 0));
                    }
                    return singles;
                })
            .value_or(std::vector<uint64_t>{});

    if (args.args.empty() and singles.empty()) {
        viua::support::errorln("nothing to disassemble");
        return 1;
    }

    auto const no_demangles =
        args.get<std::set<std::string_view>>("no-demangle")
            .value_or(std::set<std::string_view>{});
    auto const demangle_li =
        not(no_demangles.contains("li") or no_demangles.contains("any"));
    auto const demangle_mem =
        not(no_demangles.contains("mem") or no_demangles.contains("any"));

    auto const preferred_output_path = args.get<std::string_view>("out");
    auto to_file                     = std::ofstream{};
    if (preferred_output_path.has_value()) {
        auto const p = std::filesystem::path{ *preferred_output_path };
        to_file.open(p);
    }
    auto& out = (preferred_output_path.has_value() ? to_file : std::cout);

    if (singles.size()) {
        for (auto const each : singles) {
            std::println(out, "0x{:016x} {}", each, ins_to_string(each));
        }
        return 0;
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
            viua::support::errorln(elf_path,
                                   "{}: {}",
                                   viua::support::errno_name(saved_errno),
                                   viua::support::errno_desc(saved_errno));
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
        viua::support::errorln(elf_path,
                               "{}: {}",
                               viua::support::errno_name(saved_errno),
                               viua::support::errno_desc(saved_errno));
        return 1;
    }

    using Module           = viua::vm::elf::Loaded_elf;
    auto const main_module = Module::load(elf_fd);
    main_module_elf_type   = main_module.header.e_type;

    if (auto const f = main_module.find_fragment(".rodata");
        not f.has_value()) {
        viua::support::errorln(elf_path, "no .rodata section found");
        return 1;
    }
    if (auto const f = main_module.find_fragment(".strtab");
        not f.has_value()) {
        viua::support::errorln(elf_path, "no .strtab section found");
        return 1;
    }
    if (auto const f = main_module.find_fragment(".symtab");
        not f.has_value()) {
        viua::support::errorln(elf_path, "no .symtab section found");
        return 1;
    }

    auto text = std::vector<viua::arch::instruction_type>{};
    if (auto const f = main_module.find_fragment(".text"); not f.has_value()) {
        viua::support::errorln(elf_path, "no .text section found");
        return 1;
    } else {
        text = main_module.make_text_from(f->get().data);
    }

    auto entry_addr = size_t{ 0 };
    if (auto const ep = main_module.entry_point(); ep.has_value()) {
        entry_addr = *ep;
    } else {
        viua::support::warningln(elf_path, "no entry point defined");
    }

    auto const rodata = main_module.find_fragment(".rodata");
    if (rodata.has_value()) {
        out << ".section \".rodata\"\n\n";

        auto const symtab                      = main_module.symtab;
        auto extern_object_definitions_present = false;
        for (auto const& sym : main_module.symtab) {
            if (ELF64_ST_TYPE(sym.st_info) != STT_OBJECT) {
                continue;
            }

            /*
             * At this moment we just want to enumerate extern functions.
             */
            if (not is_extern(sym)) {
                continue;
            }

            auto const name = main_module.str_at(sym.st_name);
            out << ".label: [[extern]] " << name << "\n";
            extern_object_definitions_present = true;
        }
        if (extern_object_definitions_present) {
            out << "\n";
        }

        for (auto i = size_t{ 1 }; i < symtab.size(); ++i) {
            auto const& sym = symtab.at(i);

            if (ELF64_ST_TYPE(sym.st_info) != STT_OBJECT) {
                continue;
            }

            if (is_extern(sym)) {
                continue;
            }

            auto const buf =
                reinterpret_cast<char const*>(rodata->get().data.data());
            auto const sv = std::string_view{ buf + sym.st_value, sym.st_size };

            auto const is_string =
                std::all_of(sv.begin(),
                            sv.end(),
                            [](char const c) -> bool
                            { return (::isprint(c) or ::isspace(c)); });
            if (not is_string) {
                continue;
            }

            auto const off = sym.st_value;

            /*
             * Do not output anonymous symbols.
             */
            if (sym.st_name == 0) {
                continue;
            }

            auto const label = main_module.str_at(sym.st_name);

            auto const data_size = sym.st_size;

            out << "; [.rodata+0x" << std::hex << std::setw(16)
                << std::setfill('0') << off << "] to"
                << " [.rodata+0x" << std::hex << std::setw(16)
                << std::setfill('0') << (off + data_size) << "] (" << std::dec
                << data_size << " byte" << (data_size == 1 ? "" : "s") << ")\n";

            auto const sym_bind = ELF64_ST_BIND(sym.st_info);
            auto const sym_vis  = sym.st_other;
            auto const sym_is_unit_local =
                (sym_bind == STB_LOCAL and sym_vis == STV_DEFAULT);
            auto const sym_is_module_local =
                (sym_bind == STB_GLOBAL and sym_vis == STV_HIDDEN);
            auto const sym_is_global =
                (sym_bind == STB_GLOBAL and sym_vis == STV_DEFAULT);

            if (sym_is_unit_local) {
                out << ".symbol " << label << "\n";
            } else if (sym_is_module_local) {
                out << ".symbol [[global,hidden]] " << label << "\n";
            } else if (sym_is_global) {
                out << ".symbol [[global]] " << label << "\n";
            } else {
                /*
                 * Do not emit .symbol directive.
                 */
            }

            out << ".label " << label << "\n";

            out << ".object string " << viua::support::string::quoted(sv)
                << "\n\n";
        }
    }

    out << ".section \".text\"\n";

    auto extern_fn_definitions_present = false;
    for (auto const& sym : main_module.symtab) {
        if (ELF64_ST_TYPE(sym.st_info) != STT_FUNC) {
            continue;
        }

        /*
         * At this moment we just want to enumerate extern functions.
         */
        if (not is_extern(sym)) {
            continue;
        }

        auto const name      = std::string{ main_module.str_at(sym.st_name) };
        auto const safe_name = match_atom(name) ? name : ('"' + name + '"');
        out << "\n.symbol [[extern]] " << safe_name;
        extern_fn_definitions_present = true;
    }
    if (extern_fn_definitions_present) {
        out << "\n";
    }

    auto ef = main_module.name_function_at(entry_addr);
    for (auto const& sym : main_module.symtab) {
        if (ELF64_ST_TYPE(sym.st_info) != STT_FUNC) {
            continue;
        }

        if (is_extern(sym)) {
            continue;
        }
        if (is_jump_label(sym)) {
            continue;
        }

        auto const addr      = sym.st_value;
        auto const size      = sym.st_size;
        auto const no_of_ops = (size / sizeof(viua::arch::instruction_type));

        out << "\n; [.text+0x" << std::hex << std::setw(16) << std::setfill('0')
            << addr << "] to "
            << "[.text+0x" << std::setw(16) << std::setfill('0')
            << (addr + size) << "] (" << std::dec << no_of_ops << " instruction"
            << (no_of_ops ? "s" : "") << ")\n";

        /*
         * Then, the name. Marking the entry point is necessary to correctly
         * recreate the behaviour of the program.
         */
        auto const name      = std::string{ main_module.str_at(sym.st_name) };
        auto const safe_name = match_atom(name) ? name : ('"' + name + '"');

        auto const sym_bind = ELF64_ST_BIND(sym.st_info);
        auto const sym_vis  = sym.st_other;
        auto const sym_is_unit_local =
            (sym_bind == STB_LOCAL and sym_vis == STV_DEFAULT);
        auto const sym_is_module_local =
            (sym_bind == STB_GLOBAL and sym_vis == STV_HIDDEN);
        auto const sym_is_global =
            (sym_bind == STB_GLOBAL and sym_vis == STV_DEFAULT);

        if (sym_is_unit_local) {
            out << ".symbol [[local]] " << safe_name << "\n";
        } else if (sym_is_module_local) {
            out << ".symbol [[hidden]] " << safe_name << "\n";
        } else if (sym_is_global) {
            out << ".symbol " << ((ef == name) ? "[[entry_point]] " : "")
                << safe_name << "\n";
        } else {
            /*
             * Do not emit .symbol directive.
             * The symbol is not callable.
             */
        }

        out << ".label " << safe_name << "\n";

        auto own_jump_labels = std::map<size_t, Elf64_Sym const*>{};
        std::for_each(main_module.symtab.begin(),
                      main_module.symtab.end(),
                      [&own_jump_labels, addr, size](auto const& sym)
                      {
                          if (ELF64_ST_TYPE(sym.st_info) != STT_FUNC) {
                              return;
                          }
                          if (ELF64_ST_BIND(sym.st_info) != STB_LOCAL) {
                              return;
                          }
                          if (sym.st_other != STV_HIDDEN) {
                              return;
                          }
                          if (sym.st_value < addr) {
                              return;
                          }
                          if (sym.st_value >= (addr + size)) {
                              return;
                          }
                          own_jump_labels.emplace(sym.st_value, &sym);
                      });

        if (not own_jump_labels.empty()) {
            out << ".begin\n";
        }

        auto cooked_text  = Cooked_text{};
        auto const offset = (addr / sizeof(viua::arch::instruction_type));
        for (auto i = size_t{ 0 }; i < no_of_ops; ++i) {
            auto const ip     = text.at(offset + i);
            auto const opcode = viua::carve_opcode_out(ip);
            cooked_text.emplace_back(i, opcode, ip, ins_to_string(ip));
        }

        cook::demangle_arodp(cooked_text,
                             main_module.symtab,
                             main_module.strtab_of(".strtab"),
                             rodata->get().data);

        if (demangle_li) {
            /*
             * This demangles LI for long immediates; covering both integers for
             * big values, and addresses (which are always stored using the long
             * form).
             */
            cook::demangle_canonical_li(cooked_text,
                                        main_module.symtab,
                                        main_module.strtab_of(".strtab"),
                                        rodata->get().data);

            /*
             * This demangles LI for short immediates.
             */
            cook::demangle_short_li(cooked_text);
        }

        cook::demangle_addiu(cooked_text);

        if (demangle_mem) {
            cook::demangle_memory(cooked_text);
        }

        auto const physical_to_logical = cook::demangle_branches(cooked_text);

        out << "    ; <binary>          <ip>               "
               "<logical>:<physical-span>\n";
        for (auto i = size_t{}; i < cooked_text.size(); ++i) {
            auto const& [index, op, ip, s] = cooked_text.at(i);
            auto const op_addr =
                (addr
                 + (sizeof(viua::arch::instruction_type) * index.physical));

            if (own_jump_labels.contains(op_addr)) {
                auto jump_label_sym = own_jump_labels.at(op_addr);
                auto const jl_name =
                    std::string{ main_module.str_at(jump_label_sym->st_name) };
                auto const jl_safe_name =
                    match_atom(jl_name) ? jl_name : ('"' + jl_name + '"');

                out << ".label " << jl_safe_name << "\n";
            }

            out << "    ; ";
            if (ip.has_value()) {
                out << std::setw(16) << std::setfill('0') << std::hex << *ip;
            } else {
                out << std::string(16, ' ');
            }

            out << "  ";
            out << std::setw(16) << std::setfill('0') << std::hex << op_addr;

            out << "  " << std::dec << std::setw(2) << std::setfill(' ') << i
                << ":" << index.physical;
            if (index.physical_span.has_value()) {
                out << "-" << *index.physical_span;
            }
            out << "\n";

            out << "    " << s << "\n";
        }

        if (not own_jump_labels.empty()) {
            out << ".begin\n";
        }
    }

    return 0;
}
