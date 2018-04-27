/*
 *  Copyright (C) 2017, 2018 Marek Marecki
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
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <viua/types/bits.h>
#include <viua/types/exception.h>
using namespace viua::types;


/*
 * Here's a cool resource for binary arithmetic:
 * https://www.cs.cornell.edu/~tomf/notes/cps104/twoscomp.html
 */
static auto to_string(std::vector<bool> const& v, bool const with_prefix = false)
    -> std::string {
    auto oss = std::ostringstream{};

    if (with_prefix) {
        oss << "0b";
    }

    for (auto i = v.size(); i; --i) {
        oss << v.at(i - 1);
    }

    return oss.str();
}
static auto binary_expand(std::vector<bool> v, decltype(v)::size_type const n)
    -> std::vector<bool> {
    auto const expanding_value = (v.size() ? v.back() : false);
    v.reserve(n);
    while (v.size() < n) {
        v.push_back(expanding_value);
    }
    return v;
}
static auto binary_clip(
    std::vector<bool> const& bits,
    std::remove_reference_t<decltype(bits)>::size_type width) -> std::vector<bool> {
    std::vector<bool> result;
    result.reserve(width);
    std::fill_n(std::back_inserter(result), width, false);

    std::copy_n(bits.begin(), std::min(bits.size(), width), result.begin());
    result = binary_expand(result, width);

    return result;
}
static auto binary_inversion(std::vector<bool> const& v) -> std::vector<bool> {
    auto inverted = std::vector<bool>{};
    inverted.reserve(v.size());

    for (auto const each : v) {
        inverted.push_back(not each);
    }

    return inverted;
}
static auto binary_to_bool(std::vector<bool> const& v) -> bool {
    for (auto const each : v) {
        if (each) {
            return true;
        }
    }
    return false;
}
static auto binary_fill_with_zeroes(std::vector<bool> v) -> std::vector<bool> {
    for (auto i = decltype(v)::size_type{0}; i < v.size(); ++i) {
        v[i] = false;
    }
    return v;
}
static auto binary_is_negative(std::vector<bool> const& v) -> bool {
    return v.back();
}
static auto binary_last_bit_set(std::vector<bool> const& v)
    -> std::optional<std::remove_reference_t<decltype(v)>::size_type> {
    auto index_of_set = std::optional<std::remove_reference_t<decltype(v)>::size_type>{};

    for (auto i = v.size(); i; --i) {
        if (v.at(i - 1)) {
            index_of_set = (i - 1);
            break;
        }
    }

    return index_of_set;
}
static auto binary_eq(std::vector<bool> lhs, std::vector<bool> rhs) -> bool {
    lhs = binary_expand(lhs, std::max(lhs.size(), rhs.size()));
    rhs = binary_expand(rhs, std::max(lhs.size(), rhs.size()));

    for (auto i = decltype(lhs)::size_type{0}; i < lhs.size(); ++i) {
        if (lhs.at(i) != rhs.at(i)) {
            return false;
        }
    }

    // yep, they are equal
    return true;
}


static auto binary_shr(std::vector<bool> v,
                       decltype(v)::size_type const n,
                       bool const padding = false)
    -> std::pair<std::vector<bool>, std::vector<bool>> {
    auto shifted = std::vector<bool>{};
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
        auto const index_to_set            = i;
        auto const index_of_value          = i + n;
        auto const index_to_set_in_shifted = i;

        if (index_of_value < v.size()) {
            if (index_to_set_in_shifted < n) {
                shifted.at(index_to_set_in_shifted) = v.at(index_to_set);
            }
            v.at(index_to_set)   = v.at(index_of_value);
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
static auto binary_shl(std::vector<bool> v, decltype(v)::size_type const n)
    -> std::pair<std::vector<bool>, std::vector<bool>> {
    auto shifted = std::vector<bool>{};
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
        auto const index_to_set            = v.size() - i - 1;
        auto const index_of_value          = v.size() - n - i - 1;
        auto const index_to_set_in_shifted = n - i - 1;

        if (index_of_value < v.size()) {
            if (index_to_set_in_shifted < n) {
                shifted.at(index_to_set_in_shifted) = v.at(index_to_set);
            }
            v.at(index_to_set)   = v.at(index_of_value);
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


namespace viua { namespace arithmetic {
namespace wrapping {
static auto binary_increment(std::vector<bool> const& v)
    -> std::pair<bool, std::vector<bool>> {
    auto carry       = true;
    auto incremented = v;

    for (auto i = decltype(incremented)::size_type{0}; carry and i < v.size();
         ++i) {
        if (v.at(i)) {
            incremented.at(i) = false;
        } else {
            incremented.at(i) = true;
            carry             = false;
        }
    }

    return {carry, incremented};
}
static auto binary_decrement(std::vector<bool> const& v)
    -> std::pair<bool, std::vector<bool>> {
    auto borrow      = false;
    auto decremented = v;

    for (auto i = decltype(decremented)::size_type{0}; i < v.size(); ++i) {
        if (v.at(i)) {
            decremented.at(i) = false;
            borrow            = false;
            break;
        } else {
            decremented.at(i) = true;
            borrow            = true;
        }
    }

    return {borrow, decremented};
}
static auto take_twos_complement(std::vector<bool> const& v) -> std::vector<bool> {
    return binary_increment(binary_inversion(v)).second;
}


static auto binary_lte(std::vector<bool> lhs, std::vector<bool> rhs) -> bool {
    lhs = binary_expand(lhs, std::max(lhs.size(), rhs.size()));
    rhs = binary_expand(rhs, std::max(lhs.size(), rhs.size()));

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
static auto binary_lt[[maybe_unused]](std::vector<bool> lhs, std::vector<bool> rhs)
    -> bool {
    lhs = binary_expand(lhs, std::max(lhs.size(), rhs.size()));
    rhs = binary_expand(rhs, std::max(lhs.size(), rhs.size()));

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


static auto binary_addition(const std::vector<bool>& lhs, const std::vector<bool>& rhs)
    -> std::vector<bool> {
    auto result = std::vector<bool>{};
    auto const size_of_result = std::max(lhs.size(), rhs.size());
    result.reserve(size_of_result + 1);
    std::fill_n(std::back_inserter(result), size_of_result, false);

    bool carry = false;

    for (auto i = decltype(size_of_result){0}; i < size_of_result; ++i) {
        auto const from_lhs = (i < lhs.size() ? lhs.at(i) : false);
        auto const from_rhs = (i < rhs.size() ? rhs.at(i) : false);

        /*
         * lhs + rhs -> 0 + 0 -> 0
         *
         * This is the easy case.
         * Everything is zero, so we just carry the carry into the result at
         * the current position, and reset the carry flag to zero (it was
         * consumed).
         */
        if ((not from_rhs) and (not from_lhs)) {
            result.at(i) = carry;
            carry        = false;
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
         * the result bit std::string on current position.
         */
        if (from_rhs and from_lhs) {
            result.at(i) = carry;
            carry        = true;
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
static auto binary_subtraction(std::vector<bool> const& lhs, std::vector<bool> const& rhs)
    -> std::vector<bool> {
    return binary_clip(
        binary_addition(binary_expand(lhs, std::max(lhs.size(), rhs.size())),
                        take_twos_complement(
                            binary_expand(rhs, std::max(lhs.size(), rhs.size())))),
        lhs.size());
}
static auto binary_multiplication(std::vector<bool>const & lhs,
                                  std::vector<bool>const & rhs) -> std::vector<bool> {
    auto intermediates = std::vector<std::vector<bool>>{};
    intermediates.reserve(rhs.size());

    /*
     * Make sure the result is *always* has at least one entry (in case the rhs
     * is all zero bits), and that the results width is *always* the sum of
     * operands' widths.
     */
    intermediates.emplace_back(lhs.size() + rhs.size());

    for (auto i = std::remove_reference_t<decltype(lhs)>::size_type{0};
         i < rhs.size();
         ++i) {
        if (not rhs.at(i)) {
            /*
             * Multiplication by 0 just gives a long std::string of zeroes.
             * There is no reason to build all these zero-filled bit strings as
             * they will only slow things down the road when all the
             * intermediate bit strings are accumulated.
             */
            continue;
        }

        auto interm = std::vector<bool>{};
        interm.reserve(i + lhs.size());
        std::fill_n(std::back_inserter(interm), i, false);

        std::copy(lhs.begin(), lhs.end(), std::back_inserter(interm));

        intermediates.emplace_back(std::move(interm));
    }

    return std::accumulate(
        intermediates.begin(),
        intermediates.end(),
        std::vector<bool>{},
        [](std::vector<bool>const & l, std::vector<bool> const& r) -> std::vector<bool> {
            return binary_addition(l, r);
        });
}
static auto binary_division(std::vector<bool> const& dividend,
                            std::vector<bool> const& rhs) -> std::vector<bool> {
    if (not binary_to_bool(rhs)) {
        throw std::make_unique<Exception>("division by zero");
    }

    auto quotinent = std::vector<bool>{};
    auto remainder = dividend;
    auto divisor   = rhs;

    quotinent.reserve(remainder.size());
    std::fill_n(std::back_inserter(quotinent), remainder.size(), false);

    if (binary_eq(divisor, dividend)) {
        return binary_increment(quotinent).second;
    }

    auto const negative_divisor   = binary_is_negative(divisor);
    auto const negative_dividend  = binary_is_negative(dividend);
    auto negative_quotinent = false;

    if (negative_divisor) {
        divisor = take_twos_complement(divisor);
    }
    if (negative_dividend) {
        remainder = take_twos_complement(remainder);
    }
    negative_quotinent = ((negative_divisor or negative_dividend)
                          and not(negative_divisor and negative_dividend));

    while (binary_lte(divisor, remainder)) {
        remainder = binary_subtraction(remainder, divisor);
        quotinent = binary_increment(quotinent).second;
    }

    if (negative_quotinent) {
        quotinent = take_twos_complement(quotinent);
    }

    return quotinent;
}
}  // namespace wrapping
namespace checked {
static auto signed_increment(std::vector<bool> v) -> std::vector<bool> {
    auto carry       = true;
    auto incremented = v;

    for (auto i = decltype(incremented)::size_type{0}; carry and i < v.size();
         ++i) {
        if (v.at(i)) {
            incremented.at(i) = false;
        } else {
            incremented.at(i) = true;
            carry             = false;
        }
    }

    if ((not binary_is_negative(v)) and binary_is_negative(incremented)) {
        throw std::make_unique<Exception>(
            "CheckedArithmeticIncrementSignedOverflow");
    }

    return incremented;
}
static auto signed_decrement(std::vector<bool> v) -> std::vector<bool> {
    auto decremented = v;

    for (auto i = decltype(decremented)::size_type{0}; i < v.size(); ++i) {
        if (v.at(i)) {
            decremented.at(i) = false;
            break;
        } else {
            decremented.at(i) = true;
        }
    }

    if (binary_is_negative(v) and not binary_is_negative(decremented)) {
        throw std::make_unique<Exception>(
            "CheckedArithmeticDecrementSignedOverflow");
    }

    return decremented;
}
static auto take_twos_complement(std::vector<bool> const& v) -> std::vector<bool> {
    return signed_increment(binary_inversion(v));
}
static auto absolute(std::vector<bool> const& v) -> std::vector<bool> {
    if (binary_is_negative(v)) {
        return take_twos_complement(v);
    } else {
        return v;
    }
}

static auto signed_lt(std::vector<bool> lhs, std::vector<bool> rhs) {
    lhs = binary_expand(lhs, std::max(lhs.size(), rhs.size()));
    rhs = binary_expand(rhs, std::max(lhs.size(), rhs.size()));

    if (binary_is_negative(lhs) and not binary_is_negative(rhs)) {
        return true;
    }
    if (not binary_is_negative(lhs) and binary_is_negative(rhs)) {
        return false;
    }

    if (binary_is_negative(lhs)) {
        lhs = take_twos_complement(lhs);
    }
    if (binary_is_negative(rhs)) {
        rhs = take_twos_complement(rhs);
    }

    for (auto i = (lhs.size() - 1); i; --i) {
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


static auto signed_add(std::vector<bool> const& lhs, std::vector<bool> const& rhs)
    -> std::vector<bool> {
    auto result = std::vector<bool>{};
    auto const size_of_result = std::max(lhs.size(), rhs.size());
    result.reserve(size_of_result + 1);
    std::fill_n(std::back_inserter(result), size_of_result, false);

    auto carry = false;

    auto const lhs_negative = binary_is_negative(lhs);
    auto const rhs_negative = binary_is_negative(rhs);

    auto result_should_be_negative = (lhs_negative and rhs_negative);
    if (lhs_negative and (not rhs_negative) and signed_lt(rhs, lhs)) {
        result_should_be_negative = true;
    }
    if ((not lhs_negative) and rhs_negative and signed_lt(lhs, lhs)) {
        result_should_be_negative = true;
    }

    for (auto i = decltype(size_of_result){0}; i < size_of_result; ++i) {
        auto const from_lhs = (i < lhs.size() ? lhs.at(i) : false);
        auto const from_rhs = (i < rhs.size() ? rhs.at(i) : false);

        /*
         * lhs + rhs -> 0 + 0 -> 0
         *
         * This is the easy case.
         * Everything is zero, so we just carry the carry into the result at
         * the current position, and reset the carry flag to zero (it was
         * consumed).
         */
        if ((not from_rhs) and (not from_lhs)) {
            result.at(i) = carry;
            carry        = false;
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
         * the result bit std::string on current position.
         */
        if (from_rhs and from_lhs) {
            result.at(i) = carry;
            carry        = true;
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

    if (result_should_be_negative and not binary_is_negative(result)) {
        throw std::make_unique<Exception>("CheckedArithmeticAdditionSignedOverflow");
    }
    if (not result_should_be_negative and binary_is_negative(result)) {
        throw std::make_unique<Exception>("CheckedArithmeticAdditionSignedOverflow");
    }

    return result;
}
static auto signed_sub(std::vector<bool> const& lhs, std::vector<bool> const& rhs)
    -> std::vector<bool> {
    if (lhs == rhs) {
        auto result = std::vector<bool>{};
        result.reserve(lhs.size());
        std::fill_n(std::back_inserter(result), lhs.size(), false);
        return result;
    }

    auto rhs_used = std::vector<bool>{};
    try {
        rhs_used = take_twos_complement(
            binary_expand(rhs, std::max(lhs.size(), rhs.size())));
    } catch (std::unique_ptr<Exception>&) {
        throw std::make_unique<Exception>(
            "CheckedArithmeticSubtractionSignedOverflow");
    }

    try {
        return binary_clip(
            signed_add(binary_expand(lhs, std::max(lhs.size(), rhs.size())),
                       rhs_used),
            lhs.size());
    } catch (std::unique_ptr<Exception>&) {
        throw std::make_unique<Exception>(
            "CheckedArithmeticSubtractionSignedOverflow");
    }
}
static auto signed_mul(std::vector<bool> const& lhs, std::vector<bool> const& rhs)
    -> std::vector<bool> {
    auto intermediates = std::vector<std::vector<bool>>{};
    intermediates.reserve(rhs.size());

    /*
     * Make sure the result is *always* has at least one entry (in case the rhs
     * is all zero bits), and that the results width is *always* the sum of
     * operands' widths.
     */
    intermediates.emplace_back(lhs.size() + rhs.size());

    auto const lhs_negative              = binary_is_negative(lhs);
    auto const rhs_negative              = binary_is_negative(rhs);
    auto const result_should_be_negative = (lhs_negative xor rhs_negative);

    for (auto i = std::remove_reference_t<decltype(lhs)>::size_type{0};
         i < rhs.size();
         ++i) {
        if (not rhs.at(i)) {
            /*
             * Multiplication by 0 just gives a long std::string of zeroes.
             * There is no reason to build all these zero-filled bit strings as
             * they will only slow things down the road when all the
             * intermediate bit strings are accumulated.
             */
            continue;
        }

        auto interm = std::vector<bool>{};
        interm.reserve(i + lhs.size());
        std::fill_n(std::back_inserter(interm), i, false);

        std::copy(lhs.begin(), lhs.end(), std::back_inserter(interm));

        intermediates.emplace_back(std::move(interm));
    }

    /*
     * Result *MUST NOT* be empty.
     * If it would be empty checking if it is negative would result in
     * segmentation fault when binary_is_negative() would try to access last
     * element of empty bit std::string.
     */
    auto result = std::vector<bool>{};
    result.reserve(lhs.size());
    std::fill_n(std::back_inserter(result), lhs.size(), false);

    result = std::accumulate(
        intermediates.begin(),
        intermediates.end(),
        result,
        [](std::vector<bool>const & l, std::vector<bool> const& r) -> std::vector<bool> {
            /*
             * Use basic (unchecked, expanding) binary addition to accumulate
             * the result. If you used checked addition multiplication would
             * throw "checked signed addition" exceptions as overflow would
             * be detected during accumulation.
             * We don't want that so we use unchecked addition here, and
             * check for errors later.
             */
            return wrapping::binary_addition(l, r);
        });

    /*
     * We have to clip the result as it must remain fixed-size.
     * However, for overflow checking, we need the full unclipped result so we
     * just stash the clipped version here. The copy is not useless as it is
     * also used for error checking.
     */
    auto clipped = binary_clip(result, lhs.size());

    /*
     * We can't just clip the result and be done with it because the part that
     * would be discarded may contain enabled bits. So let's check if the last
     * set bit (if there are any) is out of range for the valid result. If it
     * is, make some extra checks to remove false positives (it would be too
     * easy if one simple check would suffice) and only then throw exception if
     * you must.
     */
    auto last_set = binary_last_bit_set(result);
    if (last_set and *last_set >= lhs.size()) {
        /*
         * This check is here to prevent exception being thrown for
         * negative-negative multiplication. For example, this calculation would
         * throw without it:
         *
         *      -2 * -2 = 4
         *
         * due to the fact that -2 is represented as 0b11111110 in two's
         * complement. If you multiplied 0b11111110 by itself the result would
         * be longer than 8 bits, so and last set bit would have index greater
         * than 7 so, in theory, it should be an overflow.
         *
         * In such situations, though, we should check if clipped result (which
         * would be the retuned value) is not the same as the product of
         * absolute values of lhs and rhs. If they are the same, just discard
         * the extra bits - and this will yield the correct value.
         *
         * This works for small negative values for which abs * abs would be in
         * range. For values that would produce out-of-range results the clipped
         * result will be different than abs * abs, so we check for that - and
         * throw an exception.
         *
         * It surely is not the most efficient algorithm, but it is simple and
         * easy to understand. If you ever find something better - feel free to
         * implement it. The test suite should catch your mistakes.
         */
        if ((not result_should_be_negative)
            and (lhs_negative or rhs_negative)) {
            auto lhs_abs = std::vector<bool>{};
            auto rhs_abs = std::vector<bool>{};
            try {
                lhs_abs = absolute(lhs);
                rhs_abs = absolute(rhs);
            } catch (std::unique_ptr<Exception>&) {
                /*
                 * This is why we need separate lhs_abs and rhs_abs variables
                 * initialised under a try: because they can throw exceptions on
                 * overflow when taking two's complement of the minimal value
                 * (a.k.a. negative maximum, e.g. -128 for 8 bit integers).
                 */
                throw std::make_unique<Exception>(
                    "CheckedArithmeticMultiplicationSignedOverflow");
            }
            if (clipped != signed_mul(lhs_abs, rhs_abs)) {
                throw std::make_unique<Exception>(
                    "CheckedArithmeticMultiplicationSignedOverflow");
            }
        }

        /*
         * This is to catch overflows in positive-positive multiplications where
         * both operands are quite large, e.g. 64 * 64 for for 8 bit integers.
         * It is necessary to put it here because the previous check would
         * suppress these errors, as it only fires if at least one operand is
         * negative and this check fires when neither is.
         */
        if (not(lhs_negative or rhs_negative)) {
            throw std::make_unique<Exception>(
                "CheckedArithmeticMultiplicationSignedOverflow");
        }
    }

    result = std::move(clipped);

    if (result_should_be_negative != binary_is_negative(result)) {
        throw std::make_unique<Exception>(
            "CheckedArithmeticMultiplicationSignedOverflow");
    }

    return result;
}
static auto signed_div(std::vector<bool> const& dividend, std::vector<bool> const& rhs)
    -> std::vector<bool> {
    if (not binary_to_bool(rhs)) {
        throw std::make_unique<Exception>("division by zero");
    }

    auto quotinent = std::vector<bool>{};
    auto remainder = dividend;
    auto divisor   = rhs;

    quotinent.reserve(remainder.size());
    std::fill_n(std::back_inserter(quotinent), remainder.size(), false);

    if (binary_eq(divisor, dividend)) {
        return wrapping::binary_increment(quotinent).second;
    }

    auto const negative_divisor   = binary_is_negative(divisor);
    auto const negative_dividend  = binary_is_negative(dividend);
    auto negative_quotinent = false;

    try {
        divisor            = absolute(divisor);
        remainder          = absolute(remainder);
        negative_quotinent = (negative_divisor xor negative_dividend);

        while (wrapping::binary_lte(divisor, remainder)) {
            remainder = wrapping::binary_subtraction(remainder, divisor);
            quotinent = wrapping::binary_increment(quotinent).second;
        }

        if (negative_quotinent) {
            quotinent = take_twos_complement(quotinent);
        }
    } catch (std::unique_ptr<Exception>&) {
        throw std::make_unique<Exception>("CheckedArithmeticDivisionSignedOverflow");
    }

    return quotinent;
}
}  // namespace checked
namespace saturating {
static auto signed_make_max(size_t const n) -> std::vector<bool> {
    auto v = std::vector<bool>{};
    v.reserve(n);
    v.resize(n - 1, true);
    v.push_back(false);
    return v;
}
static auto signed_make_min(size_t const n) -> std::vector<bool> {
    auto v = std::vector<bool>{};
    v.reserve(n);
    v.resize(n - 1, false);
    v.push_back(true);
    return v;
}
static auto signed_is_min(std::vector<bool> const v) -> bool {
    /*
     * Last bit must be set in two's complement for the number to be negative.
     * If it's not then clearly the number encoded is not the minimum *signed*
     * value.
     */
    if (not v.back()) {
        return false;
    }
    for (auto i = decltype(v)::size_type{0}; i < (v.size() - 1); ++i) {
        /*
         * If any bit except the last is set, then the value is not minimum.
         * This works for signed integers.
         */
        if (v.at(i)) {
            return false;
        }
    }
    return true;
}
//            static auto signed_is_max(std::vector<bool> const v) -> bool {
//                /*
//                 * Last bit must be unset in two's complement for the number
//                 to be positive.
//                 * If it's not then clearly the number encoded is not the
//                 maximum *signed* value.
//                 */
//                if (v.back()) {
//                    return false;
//                }
//                for (auto i = decltype(v)::size_type{0}; i < (v.size() - 1);
//                ++i) {
//                    /*
//                     * If any bit except the last is unset, then the value is
//                     not maximum.
//                     * This works for signed integers.
//                     */
//                    if (not v.at(i)) {
//                        return false;
//                    }
//                }
//                return true;
//            }
static auto signed_increment(std::vector<bool> v) -> std::vector<bool> {
    auto carry       = true;
    auto incremented = v;

    for (auto i = decltype(incremented)::size_type{0}; carry and i < v.size();
         ++i) {
        if (v.at(i)) {
            incremented.at(i) = false;
        } else {
            incremented.at(i) = true;
            carry             = false;
        }
    }

    if ((not binary_is_negative(v)) and binary_is_negative(incremented)) {
        incremented = signed_make_max(v.size());
    }

    return incremented;
}
static auto signed_decrement(std::vector<bool> v) -> std::vector<bool> {
    if (signed_is_min(v)) {
        return v;
    }

    auto decremented = v;

    for (auto i = decltype(decremented)::size_type{0}; i < v.size(); ++i) {
        if (v.at(i)) {
            decremented.at(i) = false;
            break;
        } else {
            decremented.at(i) = true;
        }
    }

    return decremented;
}

static auto take_twos_complement(std::vector<bool> const& v) -> std::vector<bool> {
    return signed_increment(binary_inversion(v));
}
static auto signed_lt(std::vector<bool> lhs, std::vector<bool> rhs) {
    lhs = binary_expand(lhs, std::max(lhs.size(), rhs.size()));
    rhs = binary_expand(rhs, std::max(lhs.size(), rhs.size()));

    if (binary_is_negative(lhs) and not binary_is_negative(rhs)) {
        return true;
    }
    if (not binary_is_negative(lhs) and binary_is_negative(rhs)) {
        return false;
    }

    if (binary_is_negative(lhs)) {
        lhs = take_twos_complement(lhs);
    }
    if (binary_is_negative(rhs)) {
        rhs = take_twos_complement(rhs);
    }

    for (auto i = (lhs.size() - 1); i; --i) {
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
static auto absolute(std::vector<bool> const& v) -> std::vector<bool> {
    if (binary_is_negative(v)) {
        return take_twos_complement(v);
    } else {
        return v;
    }
}

static auto signed_add(std::vector<bool> lhs, std::vector<bool> rhs) -> std::vector<bool> {
    auto result = std::vector<bool>{};
    auto const size_of_result = std::max(lhs.size(), rhs.size());
    result.reserve(size_of_result + 1);
    std::fill_n(std::back_inserter(result), size_of_result, false);

    auto carry = false;

    auto const lhs_negative = binary_is_negative(lhs);
    auto const rhs_negative = binary_is_negative(rhs);

    auto result_should_be_negative = (lhs_negative and rhs_negative);
    if (lhs_negative and (not rhs_negative) and signed_lt(rhs, lhs)) {
        result_should_be_negative = true;
    }
    if ((not lhs_negative) and rhs_negative and signed_lt(lhs, lhs)) {
        result_should_be_negative = true;
    }

    for (auto i = decltype(size_of_result){0}; i < size_of_result; ++i) {
        auto const from_lhs = (i < lhs.size() ? lhs.at(i) : false);
        auto const from_rhs = (i < rhs.size() ? rhs.at(i) : false);

        /*
         * lhs + rhs -> 0 + 0 -> 0
         *
         * This is the easy case.
         * Everything is zero, so we just carry the carry into the result at
         * the current position, and reset the carry flag to zero (it was
         * consumed).
         */
        if ((not from_rhs) and (not from_lhs)) {
            result.at(i) = carry;
            carry        = false;
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
         * the result bit std::string on current position.
         */
        if (from_rhs and from_lhs) {
            result.at(i) = carry;
            carry        = true;
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

    if (result_should_be_negative and not binary_is_negative(result)) {
        result = signed_make_min(lhs.size());
    }
    if (not result_should_be_negative and binary_is_negative(result)) {
        result = signed_make_max(lhs.size());
    }

    return result;
}
static auto signed_sub(std::vector<bool> lhs, std::vector<bool> rhs) -> std::vector<bool> {
    if (lhs == rhs) {
        auto result = std::vector<bool>{};
        result.reserve(lhs.size());
        std::fill_n(std::back_inserter(result), lhs.size(), false);
        return result;
    }

    auto rhs_used = std::vector<bool>{};
    try {
        rhs_used = take_twos_complement(
            binary_expand(rhs, std::max(lhs.size(), rhs.size())));
    } catch (std::unique_ptr<Exception>&) {
        throw std::make_unique<Exception>(
            "SaturatingArithmeticSubtractionSignedOverflow");
    }

    try {
        auto r = binary_clip(
            signed_add(binary_expand(lhs, std::max(lhs.size(), rhs.size())),
                       rhs_used),
            lhs.size());
        if (signed_is_min(rhs)) {
            r = signed_increment(r);
        }
        return r;
    } catch (std::unique_ptr<Exception>&) {
        throw std::make_unique<Exception>(
            "SaturatingArithmeticSubtractionSignedOverflow");
    }
}
static auto signed_mul(std::vector<bool> const& lhs, std::vector<bool> const& rhs)
    -> std::vector<bool> {
    auto intermediates = std::vector<std::vector<bool>>{};
    intermediates.reserve(rhs.size());

    /*
     * Make sure the result is *always* has at least one entry (in case the rhs
     * is all zero bits), and that the results width is *always* the sum of
     * operands' widths.
     */
    intermediates.emplace_back(lhs.size() + rhs.size());

    auto const lhs_negative              = binary_is_negative(lhs);
    auto const rhs_negative              = binary_is_negative(rhs);
    auto const result_should_be_negative = (lhs_negative xor rhs_negative);

    for (auto i = std::remove_reference_t<decltype(lhs)>::size_type{0};
         i < rhs.size();
         ++i) {
        if (not rhs.at(i)) {
            /*
             * Multiplication by 0 just gives a long std::string of zeroes.
             * There is no reason to build all these zero-filled bit strings as
             * they will only slow things down the road when all the
             * intermediate bit strings are accumulated.
             */
            continue;
        }

        auto interm = std::vector<bool>{};
        interm.reserve(i + lhs.size());
        std::fill_n(std::back_inserter(interm), i, false);

        std::copy(lhs.begin(), lhs.end(), std::back_inserter(interm));

        intermediates.emplace_back(std::move(interm));
    }

    /*
     * Result *MUST NOT* be empty.
     * If it would be empty checking if it is negative would result in
     * segmentation fault when binary_is_negative() would try to access last
     * element of empty bit std::string.
     */
    auto result = std::vector<bool>{};
    result.reserve(lhs.size());
    std::fill_n(std::back_inserter(result), lhs.size(), false);

    result = std::accumulate(
        intermediates.begin(),
        intermediates.end(),
        result,
        [](std::vector<bool>const & l, std::vector<bool> const& r) -> std::vector<bool> {
            /*
             * Use basic (unchecked, expanding) binary addition to accumulate
             * the result. If you used checked addition multiplication would
             * throw "checked signed addition" exceptions as overflow would
             * be detected during accumulation.
             * We don't want that so we use unchecked addition here, and
             * check for errors later.
             */
            return wrapping::binary_addition(l, r);
        });

    /*
     * We have to clip the result as it must remain fixed-size.
     * However, for overflow checking, we need the full unclipped result so we
     * just stash the clipped version here. The copy is not useless as it is
     * also used for error checking.
     */
    auto clipped = binary_clip(result, lhs.size());

    /*
     * We can't just clip the result and be done with it because the part that
     * would be discarded may contain enabled bits. So let's check if the last
     * set bit (if there are any) is out of range for the valid result. If it
     * is, make some extra checks to remove false positives (it would be too
     * easy if one simple check would suffice) and only then throw exception if
     * you must.
     */
    auto last_set = binary_last_bit_set(result);
    if (last_set and *last_set >= lhs.size()) {
        /*
         * This check is here to prevent exception being thrown for
         * negative-negative multiplication. For example, this calculation would
         * throw without it:
         *
         *      -2 * -2 = 4
         *
         * due to the fact that -2 is represented as 0b11111110 in two's
         * complement. If you multiplied 0b11111110 by itself the result would
         * be longer than 8 bits, so and last set bit would have index greater
         * than 7 so, in theory, it should be an overflow.
         *
         * In such situations, though, we should check if clipped result (which
         * would be the retuned value) is not the same as the product of
         * absolute values of lhs and rhs. If they are the same, just discard
         * the extra bits - and this will yield the correct value.
         *
         * This works for small negative values for which abs * abs would be in
         * range. For values that would produce out-of-range results the clipped
         * result will be different than abs * abs, so we check for that - and
         * throw an exception.
         *
         * It surely is not the most efficient algorithm, but it is simple and
         * easy to understand. If you ever find something better - feel free to
         * implement it. The test suite should catch your mistakes.
         */
        if ((not result_should_be_negative)
            and (lhs_negative or rhs_negative)) {
            auto lhs_abs = std::vector<bool>{};
            auto rhs_abs = std::vector<bool>{};
            try {
                lhs_abs = absolute(lhs);
                rhs_abs = absolute(rhs);
            } catch (Exception* e) {
                /*
                 * This is why we need separate lhs_abs and rhs_abs variables
                 * initialised under a try: because they can throw exceptions on
                 * overflow when taking two's complement of the minimal value
                 * (a.k.a. negative maximum, e.g. -128 for 8 bit integers).
                 *
                 * In this case, the negative-positive multiplication, we need
                 * to return the minimum representable value (as the result is
                 * deep into the negative integers teritory).
                 */
                result  = signed_make_min(lhs.size());
                clipped = signed_make_min(lhs.size());
            }
            if (clipped != signed_mul(lhs_abs, rhs_abs)) {
                result  = signed_make_min(lhs.size());
                clipped = signed_make_min(lhs.size());
            }
        }

        /*
         * This is to catch overflows in positive-positive multiplications where
         * both operands are quite large, e.g. 64 * 64 for for 8 bit integers.
         * It is necessary to put it here because the previous check would
         * suppress these errors, as it only fires if at least one operand is
         * negative and this check fires when neither is.
         */
        if (not(lhs_negative or rhs_negative)) {
            result  = signed_make_max(lhs.size());
            clipped = signed_make_max(lhs.size());
        }
    }

    result = std::move(clipped);

    if (result_should_be_negative != binary_is_negative(result)) {
        if (result_should_be_negative) {
            result = signed_make_min(lhs.size());
        } else {
            result = signed_make_max(lhs.size());
        }
    }

    return result;
}
static auto signed_div(std::vector<bool> dividend, std::vector<bool> divisor)
    -> std::vector<bool> {
    if (not binary_to_bool(divisor)) {
        throw std::make_unique<Exception>("division by zero");
    }

    auto quotinent = std::vector<bool>{};
    auto remainder = dividend;

    quotinent.reserve(remainder.size());
    std::fill_n(std::back_inserter(quotinent), remainder.size(), false);

    if (signed_is_min(divisor)) {
        /*
         * Remember that we operate on arbitrary but fixed-size integers.
         * Viua uses two's complement representation for arithmetic on bits, so
         * the most negative value is greater (in absolute terms) than the most
         * positive value. Thus, (x / minimum) equals 0 even if 'x' is maximum.
         */
        return quotinent;
    }

    if (binary_eq(divisor, dividend)) {
        return wrapping::binary_increment(quotinent).second;
    }

    auto const negative_divisor   = binary_is_negative(divisor);
    auto const negative_dividend  = binary_is_negative(dividend);
    auto const negative_quotinent = (negative_divisor xor negative_dividend);

    divisor            = absolute(divisor);
    remainder          = absolute(remainder);

    while (wrapping::binary_lte(divisor, remainder)) {
        remainder = wrapping::binary_subtraction(remainder, divisor);
        quotinent = wrapping::binary_increment(quotinent).second;
    }

    if (negative_quotinent) {
        quotinent = take_twos_complement(quotinent);
    }

    return quotinent;
}
}  // namespace saturating
}}  // namespace viua::arithmetic


std::string const viua::types::Bits::type_name = "Bits";

std::string viua::types::Bits::type() const {
    return type_name;
}

std::string viua::types::Bits::str() const {
    return to_string(underlying_array);
}

bool viua::types::Bits::boolean() const {
    return binary_to_bool(underlying_array);
}

std::unique_ptr<viua::types::Value> viua::types::Bits::copy() const {
    return std::make_unique<Bits>(underlying_array);
}

auto viua::types::Bits::size() const -> size_type {
    return underlying_array.size();
}

auto viua::types::Bits::at(size_type i) const -> bool {
    return underlying_array.at(i);
}

auto viua::types::Bits::set(size_type i, const bool value) -> bool {
    bool was               = at(i);
    underlying_array.at(i) = value;
    return was;
}

auto viua::types::Bits::clear() -> void {
    for (auto i = size_type{0}; i < underlying_array.size(); ++i) {
        set(i, false);
    }
}

auto viua::types::Bits::shl(size_type n) -> std::unique_ptr<Bits> {
    auto result      = binary_shl(underlying_array, n);
    underlying_array = std::move(result.second);
    return std::make_unique<Bits>(result.first);
}

auto viua::types::Bits::shr(size_type n, const bool padding)
    -> std::unique_ptr<Bits> {
    auto result      = binary_shr(underlying_array, n, padding);
    underlying_array = std::move(result.second);
    return std::make_unique<Bits>(result.first);
}

auto viua::types::Bits::shr(size_type n) -> std::unique_ptr<Bits> {
    return shr(n, false);
}

auto viua::types::Bits::ashl(size_type n) -> std::unique_ptr<Bits> {
    auto sign    = at(underlying_array.size() - 1);
    auto shifted = shl(n);
    set(underlying_array.size() - 1, sign);
    return shifted;
}

auto viua::types::Bits::ashr(size_type n) -> std::unique_ptr<Bits> {
    return shr(n, at(size() - 1));
}

auto viua::types::Bits::rol(size_type n) -> void {
    auto shifted      = shl(n);
    const auto offset = shifted->underlying_array.size();
    for (size_type i = 0; i < offset; ++i) {
        set(i, shifted->at(i));
    }
}

auto viua::types::Bits::ror(size_type n) -> void {
    auto shifted      = shr(n);
    const auto offset = shifted->underlying_array.size();
    for (size_type i = 0; i < offset; ++i) {
        auto target_index = (underlying_array.size() - 1 - i);
        auto source_index = (offset - 1 - i);
        set(target_index, shifted->at(source_index));
    }
}

auto viua::types::Bits::inverted() const -> std::unique_ptr<Bits> {
    return std::make_unique<Bits>(binary_inversion(underlying_array));
}

auto viua::types::Bits::increment() -> void {
    underlying_array =
        viua::arithmetic::wrapping::binary_increment(underlying_array).second;
}

auto viua::types::Bits::decrement() -> void {
    underlying_array =
        viua::arithmetic::wrapping::binary_decrement(underlying_array).second;
}

auto viua::types::Bits::wrapadd(const Bits& that) const -> std::unique_ptr<Bits> {
    return std::make_unique<Bits>(
        binary_clip(viua::arithmetic::wrapping::binary_addition(
                        underlying_array, that.underlying_array),
                    size()));
}
auto viua::types::Bits::wrapsub(const Bits& that) const -> std::unique_ptr<Bits> {
    return std::make_unique<Bits>(
        binary_clip(viua::arithmetic::wrapping::binary_subtraction(
                        underlying_array, that.underlying_array),
                    size()));
}
auto viua::types::Bits::wrapmul(const Bits& that) const -> std::unique_ptr<Bits> {
    return std::make_unique<Bits>(
        binary_clip(viua::arithmetic::wrapping::binary_multiplication(
                        underlying_array, that.underlying_array),
                    size()));
}
auto viua::types::Bits::wrapdiv(const Bits& that) const -> std::unique_ptr<Bits> {
    return std::make_unique<Bits>(
        binary_clip(viua::arithmetic::wrapping::binary_division(
                        underlying_array, that.underlying_array),
                    size()));
}


auto viua::types::Bits::checked_signed_increment() -> void {
    underlying_array =
        viua::arithmetic::checked::signed_increment(underlying_array);
}
auto viua::types::Bits::checked_signed_decrement() -> void {
    underlying_array =
        viua::arithmetic::checked::signed_decrement(underlying_array);
}
auto viua::types::Bits::checked_signed_add(const Bits& that) const
    -> std::unique_ptr<Bits> {
    return std::make_unique<Bits>(
        binary_clip(viua::arithmetic::checked::signed_add(
                        underlying_array, that.underlying_array),
                    size()));
}
auto viua::types::Bits::checked_signed_sub(const Bits& that) const
    -> std::unique_ptr<Bits> {
    return std::make_unique<Bits>(
        binary_clip(viua::arithmetic::checked::signed_sub(
                        underlying_array, that.underlying_array),
                    size()));
}
auto viua::types::Bits::checked_signed_mul(const Bits& that) const
    -> std::unique_ptr<Bits> {
    return std::make_unique<Bits>(viua::arithmetic::checked::signed_mul(
        underlying_array, that.underlying_array));
}
auto viua::types::Bits::checked_signed_div(const Bits& that) const
    -> std::unique_ptr<Bits> {
    return std::make_unique<Bits>(viua::arithmetic::checked::signed_div(
        underlying_array, that.underlying_array));
}


auto viua::types::Bits::saturating_signed_increment() -> void {
    underlying_array =
        viua::arithmetic::saturating::signed_increment(underlying_array);
}
auto viua::types::Bits::saturating_signed_decrement() -> void {
    underlying_array =
        viua::arithmetic::saturating::signed_decrement(underlying_array);
}
auto viua::types::Bits::saturating_signed_add(const Bits& that) const
    -> std::unique_ptr<Bits> {
    return std::make_unique<Bits>(
        binary_clip(viua::arithmetic::saturating::signed_add(
                        underlying_array, that.underlying_array),
                    size()));
}
auto viua::types::Bits::saturating_signed_sub(const Bits& that) const
    -> std::unique_ptr<Bits> {
    return std::make_unique<Bits>(
        binary_clip(viua::arithmetic::saturating::signed_sub(
                        underlying_array, that.underlying_array),
                    size()));
}
auto viua::types::Bits::saturating_signed_mul(const Bits& that) const
    -> std::unique_ptr<Bits> {
    return std::make_unique<Bits>(viua::arithmetic::saturating::signed_mul(
        underlying_array, that.underlying_array));
}
auto viua::types::Bits::saturating_signed_div(const Bits& that) const
    -> std::unique_ptr<Bits> {
    return std::make_unique<Bits>(viua::arithmetic::saturating::signed_div(
        underlying_array, that.underlying_array));
}

auto viua::types::Bits::operator==(const Bits& that) const -> bool {
    return (size() == that.size()
            and underlying_array == that.underlying_array);
}

template<typename T>
static auto perform_bitwise_logic(const viua::types::Bits& lhs,
                                  const viua::types::Bits& rhs)
    -> std::unique_ptr<viua::types::Bits> {
    auto result = std::make_unique<viua::types::Bits>(lhs.size());

    for (auto i = viua::types::Bits::size_type{0};
         i < std::min(lhs.size(), rhs.size());
         ++i) {
        result->set(i, T()(lhs.at(i), rhs.at(i)));
    }

    return result;
}
auto viua::types::Bits::operator|(const Bits& that) const -> std::unique_ptr<Bits> {
    return perform_bitwise_logic<std::bit_or<bool>>(*this, that);
}

auto viua::types::Bits::operator&(const Bits& that) const -> std::unique_ptr<Bits> {
    return perform_bitwise_logic<std::bit_and<bool>>(*this, that);
}

auto viua::types::Bits::operator^(const Bits& that) const -> std::unique_ptr<Bits> {
    return perform_bitwise_logic<std::bit_xor<bool>>(*this, that);
}

viua::types::Bits::Bits(std::vector<bool>&& bs) {
    underlying_array = std::move(bs);
}

viua::types::Bits::Bits(std::vector<bool> const& bs) {
    underlying_array = bs;
}

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
            underlying_array.at((size * 8) - 1
                                - ((byte_index * 8) + (7u - i))) =
                (a_byte & mask);
        }
    }
}
