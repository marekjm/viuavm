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

#ifndef VIUA_SUPPORT_SHA_HH
#define VIUA_SUPPORT_SHA_HH

#include <stdint.h>
#if defined(VIUA_OPENBSD)
#include <sys/types.h>
#include <sha1.h>
#include <sha2.h>
#else
#include <sha.h>
#include <sha256.h>
#include <sha512.h>
#endif

#include <string_view>
#include <vector>


namespace viua {
struct SHA1 {
    using context_type = SHA1_CTX;

#if defined(VIUA_OPENBSD)
    inline static constexpr auto digest_size = size_t{SHA1_DIGEST_LENGTH};
#else
    inline static constexpr auto digest_size = size_t{SHA_DIGEST_LENGTH};
#endif
    using digest_type = std::vector<uint8_t>;

    context_type ctx;

    SHA1();

    auto update(std::string_view) -> SHA1&;
    auto update(std::vector<uint8_t> const&) -> SHA1&;
    auto get() -> digest_type;
};

struct SHA256 {
    using context_type = SHA1_CTX;

#if defined(VIUA_OPENBSD)
    inline static constexpr auto digest_size = size_t{SHA1_DIGEST_LENGTH};
#else
    inline static constexpr auto digest_size = size_t{SHA_DIGEST_LENGTH};
#endif
    using digest_type = std::vector<uint8_t>;

    context_type ctx;

    SHA256();

    auto update(std::string_view) -> SHA256&;
    auto update(std::vector<uint8_t> const&) -> SHA256&;
    auto get() -> digest_type;
};

struct SHA512 {
    using context_type = SHA1_CTX;

#if defined(VIUA_OPENBSD)
    inline static constexpr auto digest_size = size_t{SHA1_DIGEST_LENGTH};
#else
    inline static constexpr auto digest_size = size_t{SHA_DIGEST_LENGTH};
#endif
    using digest_type = std::vector<uint8_t>;

    context_type ctx;

    SHA512();

    auto update(std::string_view) -> SHA512&;
    auto update(std::vector<uint8_t> const&) -> SHA512&;
    auto get() -> digest_type;
};
}  // namespace viua

#endif
