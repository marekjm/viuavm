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

#include <elf.h>
#include <stdint.h>

#include <format>
#include <string>

#include <viua/arch/elf.h>
#include <viua/support/elf.hh>


namespace viua {
auto elf_class_to_string(
    uint8_t const ei_class) -> std::string
{
    switch (ei_class) {
        case ELFCLASS64:
            return "ELF64";
        case ELFCLASS32:
            return "ELF32";
        case ELFCLASSNONE:
            return "ELFNONE";
        default:
            return "ELFINVALID";
    }
}
auto elf_class_to_string(
    uint8_t const e_ident[EI_NIDENT]) -> std::string
{
    return elf_class_to_string(e_ident[EI_CLASS]);
}

auto elf_data_to_string(
    uint8_t const ei_data) -> std::string
{
    switch (ei_data) {
        case ELFDATA2LSB:
            return "two's complement, little-endian";
        case ELFDATA2MSB:
            return "two's complement, big-endian";
        case ELFDATANONE:
        default:
            return std::format("unknown data format: {}", ei_data);
    }
}
auto elf_data_to_string(
    uint8_t const e_ident[EI_NIDENT]) -> std::string
{
    return elf_data_to_string(e_ident[EI_DATA]);
}

auto elf_osabi_to_string(
    uint8_t const ei_osabi) -> std::string
{
    switch (ei_osabi) {
        case ELFOSABI_SYSV:
            return "UNIX System V";
        case ELFOSABI_HPUX:
            return "HP-UX";
        case ELFOSABI_NETBSD:
            return "NetBSD";
        case ELFOSABI_LINUX:
            return "Linux";
        case ELFOSABI_SOLARIS:
            return "Solaris";
        case ELFOSABI_IRIX:
            return "IRIX";
        case ELFOSABI_FREEBSD:
            return "FreeBSD";
        case ELFOSABI_TRU64:
            return "TRU64 UNIX";
        case ELFOSABI_MODESTO:
            return "Novell Modesto";
        case ELFOSABI_OPENBSD:
            return "OpenBSD";
        case ELFOSABI_ARM:
            return "ARM";
        case ELFOSABI_STANDALONE:
            return "Standalone";
        default:
            return std::format("unknown OS/ABI: {}", ei_osabi);
    }
}
auto elf_osabi_to_string(
    uint8_t const e_ident[EI_NIDENT]) -> std::string
{
    return elf_osabi_to_string(e_ident[EI_OSABI]);
}

auto elf_abiversion_to_string(
    uint8_t const ei_abiversion) -> std::string
{
    return std::to_string(static_cast<unsigned int>(ei_abiversion));
}
auto elf_abiversion_to_string(
    uint8_t const e_ident[EI_NIDENT]) -> std::string
{
    return elf_abiversion_to_string(e_ident[EI_ABIVERSION]);
}

auto elf_type_to_string(
    uint16_t const e_type) -> std::string
{
    switch (e_type) {
        case ET_REL:
            return "REL (Relocatable)";
        case ET_EXEC:
            return "EXEC (Executable)";
        case ET_DYN:
            return "DYN (Shared object)";
        case ET_CORE:
            return "CORE (Core)";
        case ET_NONE:
        default:
            return std::format("None (Unknown type {})", e_type);
    }
}
auto elf_machine_to_string(
    uint16_t const e_machine) -> std::string
{
    switch (e_machine) {
        case EM_NONE:
            return "None";
        case EM_X86_64:
            return "AMD x86-64";
        case EM_AARCH64:
            return "ARM AArch64";
        case EM_RISCV:
            return "RISC-V";
        case EM_VIUAVM:
            return "Viua VM";
        default:
            break;
    }
    return "Other/Unknown machine";
}

auto sh_type_to_string(
    uint32_t const sh_type) -> std::string
{
    switch (sh_type) {
        case SHT_NULL:
            return "NULL";
        case SHT_PROGBITS:
            return "PROGBITS";
        case SHT_SYMTAB:
            return "SYMTAB";
        case SHT_STRTAB:
            return "STRTAB";
        case SHT_RELA:
            return "RELA";
        case SHT_HASH:
            return "HASH";
        case SHT_DYNAMIC:
            return "DYNAMIC";
        case SHT_NOTE:
            return "NOTE";
        case SHT_NOBITS:
            return "NOBITS";
        case SHT_REL:
            return "REL";
        case SHT_SHLIB:
            return "SHLIB";
        case SHT_DYNSYM:
            return "DYNSYM";
        case SHT_LOPROC:
            return "LOPROC";
        case SHT_HIPROC:
            return "HIPROC";
        case SHT_LOUSER:
            return "LOUSER";
        case SHT_HIUSER:
            return "HIUSER";
        default:
            return std::format("<unknown section header type: {}>", sh_type);
    }
}

auto st_type_to_string(
    uint32_t const st_info) -> std::string
{
    switch (ELF64_ST_TYPE(st_info)) {
        case STT_NOTYPE:
            return "NOTYPE";
        case STT_OBJECT:
            return "OBJECT";
        case STT_FUNC:
            return "FUNC";
        case STT_SECTION:
            return "SECTION";
        case STT_FILE:
            return "FILE";
        case STT_LOPROC:
            return "LOPROC";
        case STT_HIPROC:
            return "HIPROC";
        default:
            return std::format("<unknown symbol type: {}>", st_info);
    }
}

auto st_bind_to_string(
    uint32_t const st_info) -> std::string
{
    switch (ELF64_ST_BIND(st_info)) {
        case STB_LOCAL:
            return "LOCAL";
        case STB_GLOBAL:
            return "GLOBAL";
        case STB_WEAK:
            return "WEAK";
        case STB_LOPROC:
            return "LOPROC";
        case STB_HIPROC:
            return "HIPROC";
        default:
            return std::format("<unknown binding: {}>", st_info);
    }
}

auto st_visibility_to_string(
    uint32_t const st_other) -> std::string
{
    switch (ELF64_ST_VISIBILITY(st_other)) {
        case STV_DEFAULT:
            return "DEFAULT";
        case STV_INTERNAL:
            return "INTERNAL";
        case STV_HIDDEN:
            return "HIDDEN";
        case STV_PROTECTED:
            return "PROTECTED";
        default:
            return std::format("<unknown visibility: {}>", st_other);
    }
}

auto st_shndx_to_string(
    uint32_t const st_shndx) -> std::string
{
    switch (st_shndx) {
        case SHN_UNDEF:
            return "UND";
        case SHN_ABS:
            return "ABS";
        default:
            return std::to_string(st_shndx);
    }
}

auto p_type_to_string(
    uint32_t const p_type) -> std::string
{
    if ((p_type >= PT_LOPROC) and (p_type <= PT_HIPROC)) {
        return "RESERVED";
    }

    switch (p_type) {
        case PT_NULL:
            return "NULL";
        case PT_LOAD:
            return "LOAD";
        case PT_DYNAMIC:
            return "DYNAMIC";
        case PT_INTERP:
            return "INTERP";
        case PT_NOTE:
            return "NOTE";
        case PT_SHLIB:
            return "SHLIB";
        case PT_PHDR:
            return "PHDR";
        default:
            return std::to_string(p_type);
    }
}
}  // namespace viua
