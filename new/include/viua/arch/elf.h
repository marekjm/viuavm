/*
 *  Copyright (C) 2023 Marek Marecki
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

#include <stdint.h>

namespace viua::arch::elf {
enum class R_VIUA : uint8_t {
    R_VIUA_NONE      = 0,
    R_VIUA_JUMP_SLOT = 1,
};
}  // namespace viua::arch::elf

#endif
