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

#ifndef VIUA_VM_CORE_IO_IMPL_IO_URING_HH
#define VIUA_VM_CORE_IO_IMPL_IO_URING_HH

#include <stdint.h>

#include <liburing.h>

#include <viua/vm/io/sched.hh>


namespace viua::vm::io::impl::io_uring {
struct IO : IO_scheduler {
    inline static constexpr auto IO_URING_ENTRIES = size_t{ 4'096 };
    ::io_uring ring;

    IO();
    ~IO() override;

    auto schedule(int const, IO_request::Opcode const, io::Out)
        -> IO_request::id_type override;
    auto schedule(uint8_t* const, int const, IO_request::Opcode const, io::In)
        -> IO_request::id_type override;

    auto wait(Stack&, IO_request::id_type const) -> uint8_t* override;
};
}  // namespace viua::vm::io::impl::io_uring

#endif
