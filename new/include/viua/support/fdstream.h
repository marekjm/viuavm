/*
 *  Copyright (C) 2021-2022 Marek Marecki
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

#ifndef VIUA_SUPPORT_FDSTREAM_H
#define VIUA_SUPPORT_FDSTREAM_H

#include <unistd.h>

#include <sstream>

namespace viua::support {
struct fdstream {
    using fd_type = int;

    static constexpr struct {
    } endl{};
    using endl_type = decltype(endl);

  private:
    fd_type fd{-1};
    std::ostringstream buffer;

  public:
    inline fdstream(fd_type const d) : fd{d}
    {}

    template<typename T> auto operator<<(T const& v) -> fdstream&
    {
        buffer << v;
        return *this;
    }
    inline auto operator<<(endl_type const) -> fdstream&
    {
        buffer << "\n";
        auto const buf = buffer.str();
        write(fd, buf.c_str(), buf.size());
        buffer.str("");
        return *this;
    }
    inline auto operator<<(char const c) -> fdstream&
    {
        if (c == '\n') {
            return operator<<(endl);
        }
        buffer << c;
        return *this;
    }
};
}  // namespace viua::support

#endif
