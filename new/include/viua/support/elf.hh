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

#ifndef VIUA_SUPPORT_ELF_HH
#define VIUA_SUPPORT_ELF_HH

#include <elf.h>
#include <stdint.h>

#include <string>


namespace viua {
auto elf_class_to_string(uint8_t const) -> std::string;
auto elf_class_to_string(uint8_t const e_ident[EI_NIDENT]) -> std::string;
auto elf_data_to_string(uint8_t const) -> std::string;
auto elf_data_to_string(uint8_t const e_ident[EI_NIDENT]) -> std::string;
auto elf_osabi_to_string(uint8_t const) -> std::string;
auto elf_osabi_to_string(uint8_t const e_ident[EI_NIDENT]) -> std::string;
auto elf_abiversion_to_string(uint8_t const) -> std::string;
auto elf_abiversion_to_string(uint8_t const e_ident[EI_NIDENT]) -> std::string;
auto elf_type_to_string(uint16_t const) -> std::string;
auto elf_machine_to_string(uint16_t const) -> std::string;

auto sh_type_to_string(uint32_t const sh_type) -> std::string;
auto st_type_to_string(uint32_t const st_info) -> std::string;
auto st_bind_to_string(uint32_t const st_info) -> std::string;
auto st_visibility_to_string(uint32_t const st_other) -> std::string;
auto st_shndx_to_string(uint32_t const st_shndx) -> std::string;

auto p_type_to_string(uint32_t const) -> std::string;
}  // namespace viua

#endif
