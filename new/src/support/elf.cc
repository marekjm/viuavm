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

#include <stdint.h>
#include <elf.h>

#include <format>
#include <string>

#include <viua/support/elf.hh>


namespace viua {
auto sh_type_to_string(uint32_t const sh_type) -> std::string
{
    switch (sh_type) {
        case SHT_NULL: return "NULL";
        case SHT_PROGBITS: return "PROGBITS";
        case SHT_SYMTAB: return "SYMTAB";
        case SHT_STRTAB: return "STRTAB";
        case SHT_RELA: return "RELA";
        case SHT_HASH: return "HASH";
        case SHT_DYNAMIC: return "DYNAMIC";
        case SHT_NOTE: return "NOTE";
        case SHT_NOBITS: return "NOBITS";
        case SHT_REL: return "REL";
        case SHT_SHLIB: return "SHLIB";
        case SHT_DYNSYM: return "DYNSYM";
        case SHT_LOPROC: return "LOPROC";
        case SHT_HIPROC: return "HIPROC";
        case SHT_LOUSER: return "LOUSER";
        case SHT_HIUSER: return "HIUSER";
        default:
            return std::format("<unknown section header type: {}>", sh_type);
    }
}

auto st_type_to_string(uint32_t const st_info) -> std::string
{
    switch (ELF64_ST_TYPE(st_info)) {
        case STT_NOTYPE: return "NOTYPE";
        case STT_OBJECT: return "OBJECT";
        case STT_FUNC: return "FUNC";
        case STT_SECTION: return "SECTION";
        case STT_FILE: return "FILE";
        case STT_LOPROC: return "LOPROC";
        case STT_HIPROC: return "HIPROC";
        default:
            return std::format("<unknown symbol type: {}>", st_info);
    }
}

auto st_bind_to_string(uint32_t const st_info) -> std::string
{
    switch (ELF64_ST_BIND(st_info)) {
        case STB_LOCAL: return "LOCAL";
        case STB_GLOBAL: return "GLOBAL";
        case STB_WEAK: return "WEAK";
        case STB_LOPROC: return "LOPROC";
        case STB_HIPROC: return "HIPROC";
        default:
            return std::format("<unknown binding: {}>", st_info);
    }
}

auto st_visibility_to_string(uint32_t const st_other) -> std::string
{
    switch (ELF64_ST_VISIBILITY(st_other)) {
        case STV_DEFAULT: return "DEFAULT";
        case STV_INTERNAL: return "INTERNAL";
        case STV_HIDDEN: return "HIDDEN";
        case STV_PROTECTED: return "PROTECTED";
        default:
            return std::format("<unknown visibility: {}>", st_other);
    }
}

auto st_shndx_to_string(uint32_t const st_shndx) -> std::string
{
    switch (st_shndx) {
        case SHN_UNDEF: return "UND";
        case SHN_ABS: return "ABS";
        default:
            return std::to_string(st_shndx);
    }
}
}
