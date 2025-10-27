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

#include <viua/support/sha.hh>

namespace viua {
SHA1::SHA1()
{
#if defined(VIUA_OPENBSD)
    SHA1Init(&ctx);
#else
    SHA1_Init(&ctx);
#endif
}
auto SHA1::update(std::string_view data) -> SHA1&
{
    auto const ptr = reinterpret_cast<uint8_t const*>(data.data());
#if defined(VIUA_OPENBSD)
    SHA1Update(&ctx, ptr, data.size());
#else
    SHA1_Update(&ctx, ptr, data.size());
#endif
    return *this;
}
auto SHA1::update(std::vector<uint8_t> const& v) -> SHA1&
{
    auto sv = std::string_view{reinterpret_cast<char const*>(v.data()), v.size()};
    return update(sv);
}

auto SHA1::get() -> digest_type
{
    auto d = digest_type{};
    d.resize(digest_size);
#if defined(VIUA_OPENBSD)
    SHA1Final(d.data(), &ctx);
#else
    SHA1_Final(d.data(), &ctx);
#endif
    return d;
}
}

namespace viua {
SHA256::SHA256()
{
#if defined(VIUA_OPENBSD)
    SHA1Init(&ctx);
#else
    SHA1_Init(&ctx);
#endif
}
auto SHA256::update(std::string_view data) -> SHA256&
{
    auto const ptr = reinterpret_cast<uint8_t const*>(data.data());
#if defined(VIUA_OPENBSD)
    SHA1Update(&ctx, ptr, data.size());
#else
    SHA1_Update(&ctx, ptr, data.size());
#endif
    return *this;
}
auto SHA256::update(std::vector<uint8_t> const& v) -> SHA256&
{
    auto sv = std::string_view{reinterpret_cast<char const*>(v.data()), v.size()};
    return update(sv);
}

auto SHA256::get() -> digest_type
{
    auto d = digest_type{};
    d.resize(digest_size);
#if defined(VIUA_OPENBSD)
    SHA1Final(d.data(), &ctx);
#else
    SHA1_Final(d.data(), &ctx);
#endif
    return d;
}
}

namespace viua {
SHA512::SHA512()
{
#if defined(VIUA_OPENBSD)
    SHA1Init(&ctx);
#else
    SHA1_Init(&ctx);
#endif
}
auto SHA512::update(std::string_view data) -> SHA512&
{
    auto const ptr = reinterpret_cast<uint8_t const*>(data.data());
#if defined(VIUA_OPENBSD)
    SHA1Update(&ctx, ptr, data.size());
#else
    SHA1_Update(&ctx, ptr, data.size());
#endif
    return *this;
}
auto SHA512::update(std::vector<uint8_t> const& v) -> SHA512&
{
    auto sv = std::string_view{reinterpret_cast<char const*>(v.data()), v.size()};
    return update(sv);
}

auto SHA512::get() -> digest_type
{
    auto d = digest_type{};
    d.resize(digest_size);
#if defined(VIUA_OPENBSD)
    SHA1Final(d.data(), &ctx);
#else
    SHA1_Final(d.data(), &ctx);
#endif
    return d;
}
}
