/*
 *  Copyright (C) 2017 Marek Marecki
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
#include <functional>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <viua/types/bits.h>
#include <viua/types/exception.h>
using namespace std;
using namespace viua::types;


/*
 * Here's a cool resource for binary arithemtic: https://www.cs.cornell.edu/~tomf/notes/cps104/twoscomp.html
 */
static auto to_string(vector<bool> const& v, bool const with_prefix = false) -> string {
    ostringstream oss;

    if (with_prefix) {
        oss << "0b";
    }

    for (auto i = v.size(); i; --i) {
        oss << v.at(i - 1);
    }

    return oss.str();
}
static auto binary_clip(const vector<bool>& bits, std::remove_reference_t<decltype(bits)>::size_type width)
    -> vector<bool> {
    vector<bool> result;
    result.reserve(width);
    std::fill_n(std::back_inserter(result), width, false);

    std::copy_n(bits.begin(), std::min(bits.size(), width), result.begin());

    return result;
}
static auto binary_inversion(vector<bool> const& v) -> vector<bool> {
    auto inverted = vector<bool>{};
    inverted.reserve(v.size());

    for (auto const each : v) {
        inverted.push_back(not each);
    }

    return inverted;
}
static auto binary_increment(vector<bool> const& v) -> pair<bool, vector<bool>> {
    auto carry = true;
    auto incremented = v;

    for (auto i = decltype(incremented)::size_type{0}; carry and i < v.size(); ++i) {
        if (v.at(i)) {
            incremented.at(i) = false;
        } else {
            incremented.at(i) = true;
            carry = false;
        }
    }

    return {carry, incremented};
}
static auto binary_decrement(vector<bool> const& v) -> pair<bool, vector<bool>> {
    auto borrow = false;
    auto decremented = v;

    for (auto i = decltype(decremented)::size_type{0}; i < v.size(); ++i) {
        if (v.at(i)) {
            decremented.at(i) = false;
            borrow = false;
            break;
        } else {
            decremented.at(i) = true;
            borrow = true;
        }
    }

    return {borrow, decremented};
}
static auto binary_is_negative(vector<bool> const& v) -> bool { return v.at(v.size() - 1); }
static auto take_twos_complement(vector<bool> const& v) -> vector<bool> {
    return binary_increment(binary_inversion(v)).second;
}
static auto binary_addition(const vector<bool>& lhs, const vector<bool>& rhs) -> vector<bool> {
    vector<bool> result;
    auto size_of_result = std::max(lhs.size(), rhs.size());
    result.reserve(size_of_result + 1);
    std::fill_n(std::back_inserter(result), size_of_result, false);

    bool carry = false;

    for (auto i = decltype(size_of_result){0}; i < size_of_result; ++i) {
        const auto from_lhs = (i < lhs.size() ? lhs.at(i) : false);
        const auto from_rhs = (i < rhs.size() ? rhs.at(i) : false);

        /*
         * lhs + rhs -> 0 + 0 -> 0
         *
         * This is the easy case.
         * Everything is zero, so we just carry the carry into the result at
         * the current position, and reset the carry flag to zero (it was consumed).
         */
        if ((not from_rhs) and (not from_lhs)) {
            result.at(i) = carry;
            carry = false;
            continue;
        }

        /*
         * lhs + rhs -> 1 + 1 -> 10
         *
         * This gives us a zero on current position, and
         * carry flag in the enabled state.
         *
         * If carry was enabled before we have 0 + 1 = 1, so we should
         * enable bit on current position in the result.
         * If carry was not enabled we have 0 + 0, so we should
         * obviously leave the bit disabled in the result.
         * This means that we can just copy state of the carry flag into
         * the result bit string on current position.
         */
        if (from_rhs and from_lhs) {
            result.at(i) = carry;
            carry = true;
            continue;
        }

        /*
         * At this point either the lhs or rhs is enabled, but not both.
         * So if the carry bit is enabled this gives us 1 + 1 = 10, so
         * zero should be put in result on the current position, and
         * carry flag should be enabled.
         */
        if (carry) {
            continue;
        }

        /*
         * All other cases.
         * Either lhs or rhs is enabled, and carry is not.
         * So this is the 0 + 1 = 1 case.
         * Easy.
         * Just enable the bit in the result.
         */
        result.at(i) = true;
    }

    /*
     * If there was a carry during the last operation append it to the result.
     * Basic binary addition is expanding.
     * It can be made wrapping, checked, or saturating by "post-processing".
     */
    if (carry) {
        result.push_back(carry);
    }

    return result;
}
static auto binary_expand(vector<bool> v, decltype(v)::size_type const n) -> vector<bool> {
    v.reserve(n);
    while (v.size() < n) {
        v.push_back(false);
    }
    return v;
}
static auto binary_subtraction(vector<bool> const& lhs, vector<bool> const& rhs) -> vector<bool> {
    return binary_clip(binary_addition(binary_expand(lhs, max(lhs.size(), rhs.size())),
                                       take_twos_complement(binary_expand(rhs, max(lhs.size(), rhs.size())))),
                       lhs.size());
}
static auto binary_multiplication(const vector<bool>& lhs, const vector<bool>& rhs) -> vector<bool> {
    vector<vector<bool>> intermediates;
    intermediates.reserve(rhs.size());

    /*
     * Make sure the result is *always* has at least one entry (in case the rhs is all zero bits), and
     * that the results width is *always* the sum of operands' widths.
     */
    intermediates.emplace_back(lhs.size() + rhs.size());

    for (auto i = std::remove_reference_t<decltype(lhs)>::size_type{0}; i < rhs.size(); ++i) {
        if (not rhs.at(i)) {
            /*
             * Multiplication by 0 just gives a long string of zeroes.
             * There is no reason to build all these zero-filled bit strings as
             * they will only slow things down the road when all the intermediate
             * bit strings are accumulated.
             */
            continue;
        }

        vector<bool> interm;

        interm.reserve(interm.size() + lhs.size());
        std::fill_n(std::back_inserter(interm), i, false);

        std::copy(lhs.begin(), lhs.end(), std::back_inserter(interm));

        intermediates.emplace_back(std::move(interm));
    }

    return std::accumulate(
        intermediates.begin(), intermediates.end(), vector<bool>{},
        [](const vector<bool>& l, const vector<bool>& r) -> vector<bool> { return binary_addition(l, r); });
}
static auto binary_to_bool(vector<bool> const& v) -> bool {
    for (auto const each : v) {
        if (each) {
            return true;
        }
    }
    return false;
}
static auto binary_fill_with_zeroes(vector<bool> v) -> vector<bool> {
    for (auto i = decltype(v)::size_type{0}; i < v.size(); ++i) {
        v[i] = false;
    }
    return v;
}
static auto binary_shr(vector<bool> v, decltype(v)::size_type const n, bool const padding = false)
    -> pair<vector<bool>, vector<bool>> {
    auto shifted = vector<bool>{};
    shifted.reserve(n);
    for (auto i = decltype(n){0}; i < n; ++i) {
        shifted.push_back(false);
    }

    if (n >= v.size()) {
        for (auto i = decltype(n){0}; i < v.size(); ++i) {
            shifted.at(i) = v.at(i);
        }
        return {shifted, binary_fill_with_zeroes(std::move(v))};
    }

    for (auto i = decltype(n){0}; i < v.size(); ++i) {
        auto index_to_set = i;
        auto index_of_value = i + n;
        auto index_to_set_in_shifted = i;

        if (index_of_value < v.size()) {
            if (index_to_set_in_shifted < n) {
                shifted.at(index_to_set_in_shifted) = v.at(index_to_set);
            }
            v.at(index_to_set) = v.at(index_of_value);
            v.at(index_of_value) = padding;
        } else {
            if (index_to_set_in_shifted < n) {
                shifted.at(index_to_set_in_shifted) = v.at(index_to_set);
            }
            v.at(index_to_set) = padding;
        }
    }

    return {shifted, v};
}
static auto binary_shl(vector<bool> v, decltype(v)::size_type const n) -> pair<vector<bool>, vector<bool>> {
    auto shifted = vector<bool>{};
    shifted.reserve(n);
    for (auto i = decltype(n){0}; i < n; ++i) {
        shifted.push_back(false);
    }

    if (n >= v.size()) {
        for (auto i = decltype(n){0}; i < v.size(); ++i) {
            shifted.at(shifted.size() - 1 - i) = v.at(v.size() - 1 - i);
        }
        return {shifted, binary_fill_with_zeroes(std::move(v))};
    }

    for (auto i = decltype(n){0}; i < v.size(); ++i) {
        auto index_to_set = v.size() - i - 1;
        auto index_of_value = v.size() - n - i - 1;
        auto index_to_set_in_shifted = n - i - 1;

        if (index_of_value < v.size()) {
            if (index_to_set_in_shifted < n) {
                shifted.at(index_to_set_in_shifted) = v.at(index_to_set);
            }
            v.at(index_to_set) = v.at(index_of_value);
            v.at(index_of_value) = false;
        } else {
            if (index_to_set_in_shifted < n) {
                shifted.at(index_to_set_in_shifted) = v.at(index_to_set);
            }
            v.at(index_to_set) = false;
        }
    }

    return {shifted, v};
}
static auto binary_lte(vector<bool> lhs, vector<bool> rhs) -> bool {
    lhs = binary_expand(lhs, max(lhs.size(), rhs.size()));
    rhs = binary_expand(rhs, max(lhs.size(), rhs.size()));

    for (auto i = lhs.size(); i; --i) {
        if (lhs.at(i - 1) < rhs.at(i - 1)) {
            // definitely lhs < rhs
            return true;
        } else if (lhs.at(i - 1) > rhs.at(i - 1)) {
            // totally lhs > rhs
            return false;
        }
    }
    // equal to each other
    return true;
}
static auto binary_lt[[maybe_unused]](vector<bool> lhs, vector<bool> rhs) -> bool {
    lhs = binary_expand(lhs, max(lhs.size(), rhs.size()));
    rhs = binary_expand(rhs, max(lhs.size(), rhs.size()));

    for (auto i = lhs.size(); i; --i) {
        if (lhs.at(i - 1) < rhs.at(i - 1)) {
            // definitely lhs < rhs
            return true;
        } else if (lhs.at(i - 1) > rhs.at(i - 1)) {
            // totally lhs > rhs
            return false;
        }
    }
    // probably equal to each other
    return false;
}
static auto binary_eq(vector<bool> lhs, vector<bool> rhs) -> bool {
    lhs = binary_expand(lhs, max(lhs.size(), rhs.size()));
    rhs = binary_expand(rhs, max(lhs.size(), rhs.size()));

    for (auto i = decltype(lhs)::size_type{0}; i < lhs.size(); ++i) {
        if (lhs.at(i) != rhs.at(i)) {
            return false;
        }
    }

    // yep, they are equal
    return true;
}
static auto binary_division(vector<bool> const& dividend, vector<bool> const& rhs) -> vector<bool> {
    auto quotinent = vector<bool>{};
    auto remainder = dividend;
    auto divisor = rhs;

    quotinent.reserve(remainder.size());
    std::fill_n(std::back_inserter(quotinent), remainder.size(), false);

    if (binary_eq(divisor, dividend)) {
        return binary_increment(quotinent).second;
    }

    auto negative_divisor = binary_is_negative(divisor);
    auto negative_dividend = binary_is_negative(dividend);
    auto negative_quotinent = false;

    if (negative_divisor) {
        divisor = take_twos_complement(divisor);
    }
    if (negative_dividend) {
        remainder = take_twos_complement(remainder);
    }
    negative_quotinent =
        ((negative_divisor or negative_dividend) and not(negative_divisor and negative_dividend));

    while (binary_lte(divisor, remainder)) {
        remainder = binary_subtraction(remainder, divisor);
        quotinent = binary_increment(quotinent).second;
    }

    if (negative_quotinent) {
        quotinent = take_twos_complement(quotinent);
    }

    return quotinent;
}


const string viua::types::Bits::type_name = "Bits";

string viua::types::Bits::type() const { return type_name; }

string viua::types::Bits::str() const { return to_string(underlying_array); }

bool viua::types::Bits::boolean() const { return binary_to_bool(underlying_array); }

unique_ptr<viua::types::Value> viua::types::Bits::copy() const { return make_unique<Bits>(underlying_array); }

auto viua::types::Bits::size() const -> size_type { return underlying_array.size(); }

auto viua::types::Bits::at(size_type i) const -> bool { return underlying_array.at(i); }

auto viua::types::Bits::set(size_type i, const bool value) -> bool {
    bool was = at(i);
    underlying_array.at(i) = value;
    return was;
}

auto viua::types::Bits::clear() -> void {
    for (auto i = size_type{0}; i < underlying_array.size(); ++i) {
        set(i, false);
    }
}

auto viua::types::Bits::shl(size_type n) -> unique_ptr<Bits> {
    auto result = binary_shl(underlying_array, n);
    underlying_array = std::move(result.second);
    return make_unique<Bits>(result.first);
}

auto viua::types::Bits::shr(size_type n, const bool padding) -> unique_ptr<Bits> {
    auto result = binary_shr(underlying_array, n, padding);
    underlying_array = std::move(result.second);
    return make_unique<Bits>(result.first);
}

auto viua::types::Bits::shr(size_type n) -> unique_ptr<Bits> { return shr(n, false); }

auto viua::types::Bits::ashl(size_type n) -> unique_ptr<Bits> {
    auto sign = at(underlying_array.size() - 1);
    auto shifted = shl(n);
    set(underlying_array.size() - 1, sign);
    return shifted;
}

auto viua::types::Bits::ashr(size_type n) -> unique_ptr<Bits> { return shr(n, at(size() - 1)); }

auto viua::types::Bits::rol(size_type n) -> void {
    auto shifted = shl(n);
    const auto offset = shifted->underlying_array.size();
    for (size_type i = 0; i < offset; ++i) {
        set(i, shifted->at(i));
    }
}

auto viua::types::Bits::ror(size_type n) -> void {
    auto shifted = shr(n);
    const auto offset = shifted->underlying_array.size();
    for (size_type i = 0; i < offset; ++i) {
        auto target_index = (underlying_array.size() - 1 - i);
        auto source_index = (offset - 1 - i);
        set(target_index, shifted->at(source_index));
    }
}

auto viua::types::Bits::inverted() const -> unique_ptr<Bits> {
    return make_unique<Bits>(binary_inversion(underlying_array));
}

auto viua::types::Bits::increment() -> bool {
    auto result = binary_increment(underlying_array);
    underlying_array = std::move(result.second);
    return result.first;
}

auto viua::types::Bits::decrement() -> bool {
    auto result = binary_decrement(underlying_array);
    underlying_array = std::move(result.second);
    return result.first;
}

auto viua::types::Bits::wrapadd(const Bits& that) const -> unique_ptr<Bits> {
    return make_unique<Bits>(binary_clip(binary_addition(underlying_array, that.underlying_array), size()));
}
auto viua::types::Bits::wrapmul(const Bits& that) const -> unique_ptr<Bits> {
    return make_unique<Bits>(
        binary_clip(binary_multiplication(underlying_array, that.underlying_array), size()));
}
auto viua::types::Bits::wrapdiv(const Bits& that) const -> unique_ptr<Bits> {
    if (not that.boolean()) {
        throw new viua::types::Exception("division by zero");
    }
    return make_unique<Bits>(binary_clip(binary_division(underlying_array, that.underlying_array), size()));
}

auto viua::types::Bits::operator==(const Bits& that) const -> bool {
    return (size() == that.size() and underlying_array == that.underlying_array);
}

template<typename T>
static auto perform_bitwise_logic(const viua::types::Bits& lhs, const viua::types::Bits& rhs)
    -> unique_ptr<viua::types::Bits> {
    auto result = make_unique<viua::types::Bits>(lhs.size());

    for (auto i = viua::types::Bits::size_type{0}; i < min(lhs.size(), rhs.size()); ++i) {
        result->set(i, T()(lhs.at(i), rhs.at(i)));
    }

    return result;
}
auto viua::types::Bits::operator|(const Bits& that) const -> unique_ptr<Bits> {
    return perform_bitwise_logic<bit_or<bool>>(*this, that);
}

auto viua::types::Bits::operator&(const Bits& that) const -> unique_ptr<Bits> {
    return perform_bitwise_logic<bit_and<bool>>(*this, that);
}

auto viua::types::Bits::operator^(const Bits& that) const -> unique_ptr<Bits> {
    return perform_bitwise_logic<bit_xor<bool>>(*this, that);
}

viua::types::Bits::Bits(vector<bool>&& bs) { underlying_array = std::move(bs); }

viua::types::Bits::Bits(vector<bool> const& bs) { underlying_array = bs; }

viua::types::Bits::Bits(size_type i) {
    underlying_array.reserve(i);
    for (; i; --i) {
        underlying_array.push_back(false);
    }
}

viua::types::Bits::Bits(const size_type size, const uint8_t* source) {
    underlying_array.reserve(size * 8);
    for (auto i = size * 8; i; --i) {
        underlying_array.push_back(false);
    }
    const uint8_t one = 1;
    for (size_type byte_index = 0; byte_index < size; ++byte_index) {
        viua::internals::types::byte a_byte = *(source + byte_index);

        for (auto i = 0u; i < 8; ++i) {
            auto mask = static_cast<decltype(one)>(one << i);
            underlying_array.at((size * 8) - 1 - ((byte_index * 8) + (7u - i))) = (a_byte & mask);
        }
    }
}
