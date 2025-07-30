/*
 *  Copyright (C) 2017-2019, 2025 Marek Marecki
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

#include <algorithm>
#include <sstream>

#include <viua/support/binarith.hh>


namespace viua::arithmetic {
auto to_string(
      arithmetic_type const v
    , bool const with_prefix
    , std::optional<std::pair<size_type, char>> const separator) -> std::string
{
    auto oss = std::ostringstream{};
    for (auto i = size_type{ 0 }; i < v.size(); ++i) {
        oss << v.at(i);

        if (separator and ((i + 1) != v.size())) {
            auto const [step, mark] = *separator;

            if (((i + 1) % step) == 0) {
                oss << mark;
            }
        }
    }

    auto tmp = oss.str();
    std::reverse(tmp.begin(), tmp.end());

    return with_prefix ? ("0b" + std::move(tmp)) : std::move(tmp);
}

auto to_unsigned(arithmetic_type const v) -> uint64_t
{
    auto const fixed = extend(v, 64);
    std::bitset<64> bs;
    for (auto i = size_type{ 0 }; i < fixed.size(); ++i) {
        bs.set(i, fixed.at(i));
    }
    return bs.to_ulong();
}

auto to_signed(arithmetic_type const v) -> int64_t
{
    auto const fixed = extend(v, 64);
    std::bitset<64> bs;
    for (auto i = size_type{ 0 }; i < fixed.size(); ++i) {
        bs.set(i, fixed.at(i));
    }
    return static_cast<int64_t>(bs.to_ulong());
}

auto extend(arithmetic_type v, size_type const size, std::optional<bool> const expander) -> arithmetic_type
{
    auto const bit = expander.value_or(v.empty() ? false : v.back());
    v.resize(size, bit);
    return v;
}

auto is_negative(arithmetic_type const v) -> bool
{
    if (v.empty()) {
        return false;
    }

    return v.back();
}

namespace bits {
auto inc(arithmetic_type v) -> arithmetic_type
{
    auto carry = false;

    for (auto i = size_type{ 0 }; i < v.size(); ++i) {
        auto const bit = v[i];

        carry = bit;
        v[i] = not bit;

        if (not carry) {
            break;
        }
    }
    
    if (carry) {
        v.push_back(carry);
    }

    return v;
}

auto dec(arithmetic_type v) -> arithmetic_type
{
    auto borrow = false;

    for (auto i = size_type{ 0 }; i < v.size(); ++i) {
        auto const bit = bool{v[i]};

        borrow = not (bit xor borrow);
        v[i] = not bit;

        if (bit and not borrow) {
            break;
        }
    }

    if (borrow) {
        v.push_back(borrow);
    }

    return v;
}

auto add(arithmetic_type const lhs, arithmetic_type const rhs) -> arithmetic_type
{
    auto v = arithmetic_type{};
    v.reserve(std::max(lhs.size(), rhs.size()) + 1);
    v.resize(std::max(lhs.size(), rhs.size()));

    auto carry = false;

    for (auto i = size_type{ 0 }; i < v.size(); ++i) {
        auto const bl = (i < lhs.size()) ? lhs[i] : false;
        auto const br = (i < rhs.size()) ? rhs[i] : false;

        if (bl and br) {
            v[i] = carry;
            carry = true;
        } else if (bl and not br) {
            if (carry) {
                v[i] = false;
                carry = true;
            } else {
                v[i] = true;
                carry = false;
            }
        } else if ((not bl) and br) {
            if (carry) {
                v[i] = false;
                carry = true;
            } else {
                v[i] = true;
                carry = false;
            }
        } else if ((not bl) and (not br)) {
            if (carry) {
                v[i] = true;
                carry = false;
            } else {
                /* do nothing */
            }
        } else {
            /* Impossible. */
            abort();
        }
    }

    if (carry) {
        v.push_back(carry);
    }

    return v;
}
}
}
