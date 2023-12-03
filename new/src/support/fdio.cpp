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

#include <stdint.h>
#include <unistd.h>

#include <string_view>

namespace viua::support::posix {
auto whole_read(int const fd, void* const buf, size_t const count) -> void
{
    auto done = ssize_t{0};
    do {
        if (auto const n = read(fd, static_cast<std::byte*>(buf) + done, count);
            n != -1) {
            done += n;
        }
    } while (done < static_cast<ssize_t>(count));
}
auto whole_write(int const fd, void const* buf, size_t const count) -> void
{
    auto done = ssize_t{0};
    do {
        if (auto const n =
                write(fd, static_cast<std::byte const*>(buf) + done, count);
            n != -1) {
            done += n;
        }
    } while (done < static_cast<ssize_t>(count));
}
}  // namespace viua::support::posix
