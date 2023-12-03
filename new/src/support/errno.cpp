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

#include <string.h>

#include <array>
#include <string>

namespace viua::support {
using strerrordesc_type = std::array<char, (sizeof("Unknown error nnn") * 2)>;

namespace {
/*
 * Overload for XSI-compliant strerror_r(3).
 *
 * This is only needed for musl on Alpine.
 */
auto errno_desc_impl
    [[maybe_unused]] (int const e, int const r, strerrordesc_type const& buf)
    -> std::string
{
    return (r == 0) ? std::string{buf.data()}
                    : ("Unknown error " + std::to_string(e));
}

/*
 * Overload for GNU-specific strerror_r(3).
 */
auto errno_desc_impl
    [[maybe_unused]] (int const, char* const txt, strerrordesc_type const&)
    -> std::string
{
    return std::string{txt};
}
}  // namespace

auto errno_name(int const e) -> std::string
{
    /*
     * In theory, checking for _GNU_SOURCE should be enough (see the strerror(3)
     * page), but musl defines _GNU_SOURCE without actually providing all GNU
     * extensions.
     */
#if defined(VIUA_PLATFORM_HAS_FEATURE_STRERRORNAME_NP)
    return std::string{strerrorname_np(e)};
#else
    switch (e) {
    case EINVAL:
        return "EINVAL";
    default:
        return "ERROR";
    }
#endif
}
auto errno_desc(int const e) -> std::string
{
    strerrordesc_type buf{};
    return errno_desc_impl(e, strerror_r(e, buf.data(), buf.size()), buf);
}
}  // namespace viua::support
