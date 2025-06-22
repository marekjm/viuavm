/*
 *  Copyright (C) 2023, 2025 Marek Marecki
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

#ifndef VIUA_ARCH_ELF_H
#define VIUA_ARCH_ELF_H

#include <elf.h>
#include <stdint.h>

#include <string_view>


namespace viua::arch::elf {
enum class R_VIUA : uint8_t
{
    R_VIUA_NONE      = 0,
    R_VIUA_JUMP_SLOT = 1,
    R_VIUA_OBJECT    = 2,
};

/*
 * Eight bytes are used because that is what we have available in the
 * Elf64_Phdr::p_paddr field, where the magic number is stored. See elf(5) for
 * more information.
 *
 * If this value is ever changed, remember to adjust the binfmt.d/viua-exec.conf
 * file responsible for proper detection of Viua ELF files.
 */
constexpr auto VIUA_MAGIC =
    std::string_view{ "\x7fVIUA\x00\x00\x00", sizeof(Elf64_Phdr::p_offset) };

/*
 * See elf(5) for more information about the Elf64_Ehdr structure, and its
 * e_machine field.
 *
 * See the comment about "new unofficial EM_* values" in the elf.h header file
 * for more information about how one should choose values for unofficial
 * machines. The advice is to "pick large random numbers".
 */
constexpr inline auto EM_VIUAVM = uint16_t{ 0x69'f2 };
}  // namespace viua::arch::elf

/*
 * The various EM_* values from <elf.h> are available in the global namespace,
 * so let's put our value in the global namespace too.
 */
using viua::arch::elf::EM_VIUAVM;

#endif
