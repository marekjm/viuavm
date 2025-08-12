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
#include <numeric>
#include <print>
#include <sstream>

#include <viua/support/binarith.hh>


/*
 * Just the arithmetic_type ie, the base type for everything else in this
 * module.
 */
namespace viua::arithmetic {
auto arithmetic_type::of_size(
    size_type const size,
    bit_type const bit) -> arithmetic_type
{
    return extend(arithmetic_type{}, size, bit);
}
auto arithmetic_type::zero(
    size_type const size) -> arithmetic_type
{
    return of_size(size, false);
}

arithmetic_type::operator bool() const
{
    return std::find(std::begin(n), std::end(n), true) != std::end(n);
}

auto arithmetic_type::size() const -> size_type
{
    return n.size();
}

auto arithmetic_type::at(
    size_type const i) const -> bool
{
    return n.at(i);
}

auto arithmetic_type::push_back(
    bit_type const bit) -> void
{
    n.push_back(bit);
}

auto operator<<(
    arithmetic_type const v,
    size_t const shift) -> arithmetic_type
{
    auto a = arithmetic_type::zero(shift);
    std::copy(v.n.begin(), v.n.end(), std::back_inserter(a.n));
    return a;
}
}  // namespace viua::arithmetic


/*
 * Details of the signed_type.
 */
namespace viua::arithmetic {
signed_type::operator bool() const
{
    return static_cast<bool>(n);
}

auto signed_type::operator~() const -> signed_type
{
    return signed_type{ take_twos_complement(n) };
}

auto signed_type::size() const -> size_type
{
    return n.size();
}
auto signed_type::in_range(
    size_type const width) const -> bool
{
    if (width == 0) {
        return false;
    }

    /*
     * A smaller sized integer is always representable in a bigger sized
     * integer.
     */
    if (size() <= width) {
        return true;
    }

    /*
     * A zero is always a zero, so we can take the easy way out if one is found.
     */
    if (not static_cast<bool>(*this)) {
        return true;
    }

    /*
     * If the expected width is smaller than the current width, we have to check
     * if all the bits after the would-be sign bit are the same.
     *
     * Why? Becuase it ensures that we can cut the number of bits to the
     * expected size, but preserve the value represented by those bits. This is
     * only possible with a consistent pattern of bits.
     */
    auto sign_bit = n.at(width - 1);
    for (auto i = width; i < size(); ++i) {
        if (n.at(i) != sign_bit) {
            return false;
        }
    }

    return true;
}

auto signed_type::sign() const -> int
{
    auto sign_bit = n[size() - 1];

    if (sign_bit) {
        return -1;
    }

    if (static_cast<bool>(*this)) {
        return 1;
    }

    return 0;
}

auto signed_type::max(
    size_type const size) -> signed_type
{
    auto v     = arithmetic_type::of_size(size, true);
    v.n.back() = false;
    return signed_type{ v };
}

auto signed_type::min(
    size_type const size) -> signed_type
{
    auto v     = arithmetic_type::of_size(size, false);
    v.n.back() = true;
    return signed_type{ v };
}

auto signed_type::zero(
    size_type const size) -> signed_type
{
    return signed_type{ arithmetic_type::zero(size) };
}

auto operator<(
    signed_type const v,
    zero_type const) -> bool
{
    return v.n.n.back() == true;
}
auto operator>(
    signed_type const v,
    zero_type const) -> bool
{
    return ((not(v < zero_type{})) and static_cast<bool>(v));
}

auto operator<(
    signed_type const lhs,
    signed_type const rhs) -> bool
{
    if (lhs.size() != rhs.size()) {
        throw std::runtime_error{ "lt: mismatched bit widths" };
    }

    auto const lhs_is_negative = lhs < zero_type{};
    auto const rhs_is_negative = rhs < zero_type{};
    if (lhs_is_negative and not rhs_is_negative) {
        return true;
    }
    if (rhs_is_negative and not lhs_is_negative) {
        return false;
    }

    /*
     * At this point we are sure that both values have the same sign. This means
     * we can convert them both to positive values and compare those, using a
     * simple algorithm, with one important caveat.
     *
     * Let's consider two examples:
     *
     *   original |  working  |           |
     *  ----+-----+-----+-----+ operation + result
     *  lhs | rhs | lhs | rhs |           |
     *  ----+-----+-----+-----+-----------+-------
     *    6 |   9 |  6  |  9  |   6 < 9   | true
     *  ----+-----+-----+-----+-----------+-------+
     *   -6 |  -9 |  6  |  9  |   6 < 9   | true  | THIS WOULD BE WRONG!
     *  ----+-----+-----+-----+-----------+-------+
     *   -6 |  -9 |  9  |  6  |   9 < 6   | false | LHS AND RHS MUST BE SWAPPED!
     *
     */
    auto const negative_numbers = lhs_is_negative;
    auto const working_lhs      = negative_numbers ? ~rhs : lhs;
    auto const working_rhs      = negative_numbers ? ~lhs : rhs;

    for (auto i = working_lhs.size(); i > 0; --i) {
        auto const lb = working_lhs.n.at(i - 1);
        auto const rb = working_rhs.n.at(i - 1);

        if (lb < rb) {
            return true;
        }
        if (lb > rb) {
            return false;
        }
    }

    return false;
}
auto operator==(
    signed_type const lhs,
    signed_type const rhs) -> bool
{
    if (lhs.size() != rhs.size()) {
        throw std::runtime_error{ "lt: mismatched bit widths" };
    }

    for (auto i = size_type{ 0 }; i < lhs.size(); ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}
}  // namespace viua::arithmetic

/*
 * Details of the unsigned_type.
 */
namespace viua::arithmetic {
unsigned_type::operator bool() const
{
    return static_cast<bool>(n);
}

auto unsigned_type::size() const -> size_type
{
    return n.size();
}

auto unsigned_type::max(
    size_type const size) -> unsigned_type
{
    return unsigned_type{ arithmetic_type::of_size(size, true) };
}

auto unsigned_type::min(
    size_type const size) -> unsigned_type
{
    return zero(size);
}

auto unsigned_type::zero(
    size_type const size) -> unsigned_type
{
    return unsigned_type{ arithmetic_type::zero(size) };
}
}  // namespace viua::arithmetic


/*
 * Implementation of wrapping arithmetic.
 */
namespace viua::arithmetic::fixed {
auto make_arithmetic(
    int64_t const v,
    size_t const width) -> signed_type
{
    return signed_type{ extend(arithmetic_type{ v }, width) };
}

auto operator+(
    signed_type const lhs,
    signed_type const rhs) -> signed_type
{
    return signed_type{ extend(bits::add(lhs.n, rhs.n), lhs.size()) };
}

auto operator-(
    signed_type const lhs,
    signed_type const rhs) -> signed_type
{
    return signed_type{ extend(bits::sub(lhs.n, rhs.n), lhs.size()) };
}

auto operator*(
    signed_type const lhs,
    signed_type const rhs) -> signed_type
{
    return signed_type{ extend(bits::mul(lhs.n, rhs.n), lhs.size()) };
}
}  // namespace viua::arithmetic::fixed


/*
 * Implementation of saturating arithmetic.
 */
namespace viua::arithmetic::saturating {
constexpr auto DEBUG_SATURATING = false;

auto make_arithmetic(
    int64_t const v,
    size_t const width) -> signed_type
{
    auto const raw = signed_type{ arithmetic_type{ v } };
    auto const val = signed_type{ extend(raw.n, width) };

    if (raw.in_range(width)) {
        return val;
    }

    /*
     * If the value is not in range there are only two options we have to
     * consider: either the maximum or the minimum has been exceeded.
     *
     * It is not actually necessary to know what the exact original value was,
     * as we are not able to represent it anyway, and just knowing the sign is
     * enough to produce correct behaviour.
     *
     * If the value was too, big return the maximum; if it was too small, return
     * the minimum. Et voila!
     */
    return (raw.sign() == 1) ? signed_type::max(width)
                             : signed_type::min(width);
}

auto operator+(
    signed_type const lhs,
    signed_type const rhs) -> signed_type
{
    auto const lhs_is_neg = lhs < zero_type{};
    auto const lhs_is_zer = not lhs;
    auto const rhs_is_neg = rhs < zero_type{};
    auto const rhs_is_zer = not rhs;

    /*
     * Expect Greater or Equal to Zero only of both sides are non-negative.
     */
    auto const expect_gez = (not lhs_is_neg) and (not rhs_is_neg);

    /*
     * Expect Less Than Zero in several cases.
     */
    auto const expect_ltz =
        /*
         * ...when the left hand side is negative or zero, and right hand side
         * is negative. For example:
         *       0 + -1
         *      -1 + -1
         */
        ((lhs_is_neg or lhs_is_zer) and rhs_is_neg)
        /*
         * ...when left hand side is negative, and right hand size is zero.
         */
        or (lhs_is_neg and rhs_is_zer)
        /*
         * ...when left hand side is negative, right hand side is non-negative,
         * and the right hand side is less than the twos-complement of left hand
         * side. For example:
         *      -2 + 1  (because 1 < abs(-2))
         */
        or (lhs_is_neg and (not rhs_is_neg) and (rhs < ~lhs))
        /*
         * ...when left hand side is positive, right hand side is negative,
         * and the left hand side is less than the twos-complement of right hand
         * side. For example:
         *      1 + -3  (because 1 < abs(-3))
         */
        or ((not lhs_is_neg) and rhs_is_neg and (lhs < ~rhs));

    /*
     * Raw result of the operation, assuming infinite-width integers.
     */
    auto const raw        = signed_type{ bits::add(lhs.n, rhs.n) };
    auto const raw_is_neg = raw < zero_type{};

    /*
     * Now let's detect overflow.
     *
     * One indicator of possible overflow is an "oversized" result. This,
     * however, is not always sufficient to prove overflow--and sometimes would
     * even lead to a false positive--so we catch true overflow separately.
     *
     * Detecting overflow is not that difficult. Basically, we have to know what
     * is the expected sign of the result (this is why we have determined that
     * earlier), and see if what we got is what we expected. If yes, splendid;
     * otherwise, we need to fix the situation.
     */
    auto const oversize = (raw.size() > lhs.size());
    auto const overflow =
        (expect_gez and raw_is_neg) or (expect_ltz and (not raw_is_neg));


    if constexpr (DEBUG_SATURATING) {
        std::println("saturating+ state:");
        std::println("  lhs: {} ({})",
                     to_string(lhs, false, DEFAULT_SEPARATOR),
                     static_cast<int8_t>(lhs));
        std::println("  rhs: {} ({})",
                     to_string(rhs, false, DEFAULT_SEPARATOR),
                     static_cast<int8_t>(rhs));
        std::println("  lin: {}", lhs_is_neg);
        std::println("  rin: {}", rhs_is_neg);
        std::println("  gez: {}", expect_gez);
        std::println("  ltz: {}", expect_ltz);
        std::println("  ovs: {}", oversize);
        std::println("  ovf: {}", overflow);
        std::println("  raw: {} ({})",
                     to_string(raw, false, DEFAULT_SEPARATOR),
                     static_cast<int8_t>(raw));
    }

    if (oversize) {
        auto const clipped        = signed_type{ extend(raw.n, lhs.size()) };
        auto const clipped_is_neg = clipped < zero_type{};

        if (expect_ltz and not clipped_is_neg) {
            return signed_type::min(lhs.size());
        }

        if (expect_gez and clipped_is_neg) {
            return signed_type::max(lhs.size());
        }

        return clipped;
    }

    /*
     * Both operands are non-negative, so we know what to do in case of
     * overflow: return the upper limit.
     */
    if (overflow and expect_gez) {
        return signed_type::max(lhs.size());
    }

    if (overflow and expect_ltz) {
        return signed_type::min(lhs.size());
    }

    return raw;
}

auto operator-(
    signed_type const lhs,
    signed_type const rhs) -> signed_type
{
    /*
     * Check for the (X - X) case and return early to simplify the rest of the
     * function.
     */
    if (lhs == rhs) {
        return signed_type::zero(lhs.size());
    }

    auto const lhs_is_neg = lhs < zero_type{};
    auto const rhs_is_neg = rhs < zero_type{};

    /*
     * Expect Less Than Zero in several cases.
     */
    auto const expect_ltz =
        /*
         * ...when the left hand side is negative and the right hand side is
         * zero. For example:
         *      -1 - 0
         */
        (lhs_is_neg and (not rhs))
        /*
         * ...when the left hand side is negative, the right hand side is
         * negative, and the left hand side is less than the right hand side.
         * For example:
         *      -3 - -1     (because (-3 + 1) < 0)
         */
        or (lhs_is_neg and rhs_is_neg and (lhs < rhs))
        /*
         * ...when the left hand side is zero, and the right hand side is
         * greater than zero. For example:
         *      0 - 1
         */
        or ((not lhs) and (rhs > zero_type{}))
        /*
         * ...when the left hand side is non-negative, the right hand side is
         * non-negative, and the left hand side is less than the right hand
         * side. For example:
         *      1 - 2
         */
        or ((not lhs_is_neg) and (not rhs_is_neg) and (lhs < rhs))
        /*
         * ...when the left hand side is negative, and the right hand side is
         * non-negative. For example:
         *      -1 - 1
         */
        or (lhs_is_neg and (not rhs_is_neg));

    /*
     * Expect Greater Than Zero in other cases.
     */
    auto const expect_gtz = not expect_ltz;

    /*
     * Raw result of the operation, assuming infinite-width integers.
     */
    auto const raw = signed_type{ bits::sub(lhs.n, rhs.n) };

    /*
     * Now let's detect overflow.
     */
    auto const oversize = (raw.size() > lhs.size());
    auto const overflow = (expect_gtz and (raw < zero_type{}))
                          or (expect_ltz and not(raw < zero_type{}));


    if constexpr (DEBUG_SATURATING) {
        std::println("saturating- state:");
        std::println("  lhs: {} ({})",
                     to_string(lhs, false, DEFAULT_SEPARATOR),
                     static_cast<int8_t>(lhs));
        std::println("  rhs: {} ({})",
                     to_string(rhs, false, DEFAULT_SEPARATOR),
                     static_cast<int8_t>(rhs));
        std::println("  lin: {}", lhs_is_neg);
        std::println("  rin: {}", rhs_is_neg);
        std::println("  gtz: {}", expect_gtz);
        std::println("  ltz: {}", expect_ltz);
        std::println("  ovs: {}", oversize);
        std::println("  ovf: {}", overflow);
        std::println("  raw: {} ({})",
                     to_string(raw, false, DEFAULT_SEPARATOR),
                     static_cast<int8_t>(raw));
    }

    if (oversize) {
        auto const clipped = signed_type{ extend(raw.n, lhs.size()) };

        if (expect_ltz and not(clipped < zero_type{})) {
            return signed_type::min(lhs.size());
        }

        if (expect_gtz and (clipped < zero_type{})) {
            return signed_type::max(lhs.size());
        }

        return clipped;
    }

    if (overflow and expect_gtz) {
        return signed_type::max(lhs.size());
    }

    if (overflow and expect_ltz) {
        return signed_type::min(lhs.size());
    }

    return raw;
}

auto operator*(
    signed_type const lhs,
    signed_type const rhs) -> signed_type
{
    auto const raw = signed_type{ bits::mul(lhs.n, rhs.n) };

    /*
     * Detect zero early. Not having to deal with a zero and being able to only
     * consider negative or positive numbers makes the algorithm surprisingly
     * simpler. I did not expect it to be so, but someties life can be
     * surprising.
     */
    if (not static_cast<bool>(raw)) {
        return signed_type::zero(lhs.size());
    }

    /*
     * At this point we are sure that we are dealing with non-zero values. First
     * thing we should do is determine the expected sign of the result, as it is
     * the easiest (but not foolproof!) way of determining whether or not
     * overflow happened.
     */
    auto const lhs_negative                 = lhs < zero_type{};
    auto const rhs_negative                 = rhs < zero_type{};
    auto const expect_negative              = lhs_negative xor rhs_negative;
    auto const expect_sign [[maybe_unused]] = expect_negative ? -1 : 1;

    /*
     * Cut the raw value to target size plus one, to detect if a carry happened.
     * This is another easy (but, again, not foolproof!) way of spotting cases
     * where we need to saturate.
     */
    auto const car = signed_type{ extend(raw.n, lhs.size() + 1) };
    auto const val = signed_type{ extend(raw.n, lhs.size()) };

    if constexpr (true) {
        std::println("saturating operator*:");
        std::println("  lhs: {}", to_string(lhs, false, DEFAULT_SEPARATOR));
        std::println("  rhs: {}", to_string(rhs, false, DEFAULT_SEPARATOR));
        std::println("  raw: {} ({})",
                     to_string(raw, false, DEFAULT_SEPARATOR),
                     static_cast<int16_t>(raw));
        std::println("  car: {} (in range)",
                     to_string(car, false, DEFAULT_SEPARATOR),
                     (car.in_range(lhs.size()) ? "" : "not "));
        std::println("  val: {}", to_string(val, false, DEFAULT_SEPARATOR));
        std::println("  sign:");
        std::println("    lhs: {:2d}", lhs_negative ? -1 : 1);
        std::println("    rhs: {:2d}", rhs_negative ? -1 : 1);
        std::println("    exp: {:2d}", expect_negative ? -1 : 1);
        std::println("    raw: {:2d}", raw.sign());
        std::println("    car: {:2d}", car.sign());
        std::println("    val: {:2d}", val.sign());
    }

    /*
     * Sometimes, the value is in range, but the sign is incorrect. This can
     * happen when two negative numbers are multiplied. Consider:
     *
     *      -1  *sat8  -128
     *
     * The left hand operand (-1) is 1111'1111; and the right hand operand
     * (-128) is 1000'000. They are both negative, so the expected sign of the
     * result is positive.
     *
     * However, notice what the car and val look like in this case:
     *
     *      car  1'1000'0000
     *      val    1000'0000
     *
     * The car is simply sign-extended val! Obviously, this means that car fits
     * perfectly in our target range, so no overflow happened. This is an
     * incorrect conclusion, since val is negative while we expect to get a
     * positive result.
     *
     * Thus the need to make sure that the sign of the value actually matches
     * what we expect, even if the value is seemingly in range.
     */
    if (car.in_range(lhs.size()) and (val.sign() == expect_sign)) {
        return val;
    }

    /*
     * Consider the case of:
     *
     *      127  *sat8  127
     */
    if ((car.sign() == 1) and (val.sign() == expect_sign)) {
        return val;
    }

    /*
     * In case of overflow, just return the positive or negative maximum, as
     * necessary depending on the expected sign.
     */
    return expect_negative ? signed_type::min(lhs.size())
                           : signed_type::max(lhs.size());
}
}  // namespace viua::arithmetic::saturating


/*
 * Implementation of arithmetic on unlimited-width integers.
 */
namespace viua::arithmetic::bits {
auto inc(
    arithmetic_type v) -> arithmetic_type
{
    auto carry = false;

    for (auto i = size_type{ 0 }; i < v.size(); ++i) {
        auto const bit = v.at(i);

        carry  = bit;
        v.n[i] = not bit;

        if (not carry) {
            break;
        }
    }

    if (carry) {
        v.push_back(carry);
    }

    return v;
}

auto dec(
    arithmetic_type v) -> arithmetic_type
{
    auto borrow = false;

    for (auto i = size_type{ 0 }; i < v.size(); ++i) {
        auto const bit = v.at(i);

        borrow = not(bit xor borrow);
        v.n[i] = not bit;

        if (bit and not borrow) {
            break;
        }
    }

    if (borrow) {
        v.push_back(borrow);
    }

    return v;
}

auto add(
    arithmetic_type const lhs,
    arithmetic_type const rhs) -> arithmetic_type
{
    if (not rhs) {
        return lhs;
    }
    if (not lhs) {
        return rhs;
    }

    auto v = arithmetic_type{};
    v.n.reserve(std::max(lhs.size(), rhs.size()) + 1);
    v.n.resize(std::max(lhs.size(), rhs.size()));

    auto carry = false;

    for (auto i = size_type{ 0 }; i < v.size(); ++i) {
        auto const bl = (i < lhs.size()) ? lhs.at(i) : false;
        auto const br = (i < rhs.size()) ? rhs.at(i) : false;

        if (bl xor br) {
            v.n[i] = not carry;
        } else {
            v.n[i] = carry;
            carry  = bl and br;
        }
    }

    if (carry) {
        v.push_back(carry);
    }

    return v;
}

auto sub(
    arithmetic_type const lhs,
    arithmetic_type const rhs) -> arithmetic_type
{
    if (not rhs) {
        return lhs;
    }
    if (not lhs) {
        return take_twos_complement(rhs);
    }

    return add(lhs, take_twos_complement(rhs));
}

auto mul(
    arithmetic_type const lhs,
    arithmetic_type const rhs) -> arithmetic_type
{
    auto intermediates = std::vector<arithmetic_type>{};
    intermediates.reserve(rhs.size() + 1);

    /*
     * Make sure the result is *always* has at least one entry (in case the rhs
     * is all zero bits), and that the results width is *always* the sum of
     * operands' widths.
     */
    intermediates.emplace_back(arithmetic_type::zero(lhs.size() + rhs.size()));

    for (auto i = size_type{ 0 }; i < rhs.size(); ++i) {
        if (not rhs[i]) {
            /*
             * Multiplication by zero is always zero, so this bit can be
             * skipped. There is no reason to accumulate intermediate zero
             * results and slow things down.
             */
            continue;
        }

        intermediates.emplace_back(lhs << i);
    }

    return std::accumulate(intermediates.begin(),
                           intermediates.end(),
                           arithmetic_type::zero(lhs.size() + rhs.size()),
                           [](arithmetic_type const& l,
                              arithmetic_type const& r) -> arithmetic_type
                           { return bits::add(l, r); });
}
}  // namespace viua::arithmetic::bits


/*
 * Utilities, helpers, etc.
 */
namespace viua::arithmetic {
auto to_string(
    arithmetic_type const v,
    bool const with_prefix,
    std::optional<std::pair<size_type, char>> const separator) -> std::string
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

auto extend(
    arithmetic_type v,
    size_type const size,
    std::optional<bool> const expander) -> arithmetic_type
{
    auto const bit = expander.value_or(v.n.empty() ? false : v.n.back());
    v.n.resize(size, bit);
    return v;
}

auto invert(
    arithmetic_type v) -> arithmetic_type
{
    for (auto i = size_type{ 0 }; i < v.size(); ++i) {
        v.n[i] = not v.n[i];
    }
    return v;
}

auto take_twos_complement(
    arithmetic_type const v) -> arithmetic_type
{
    return bits::inc(invert(v));
}
}  // namespace viua::arithmetic

namespace viua::arithmetic::bits {
#if 0
auto lt(arithmetic_type const lhs, arithmetic_type const rhs) -> bool
{
    if (lhs.size() != rhs.size()) {
        throw std::runtime_error{"lt: mismatched bit widths"};
    }

    auto const lhs_is_negative = lhs < zero_type{};
    auto const rhs_is_negative = rhs < zero_type{};
    if (lhs_is_negative and not rhs_is_negative) {
        return true;
    }
    if (rhs_is_negative and not lhs_is_negative) {
        return false;
    }

    /*
     * At this point we are sure that both values have the same sign. This means
     * we can convert them both to positive values and compare those, using a
     * simple algorithm, with one important caveat.
     *
     * Let's consider two examples:
     *
     *   original |  working  |           |
     *  ----+-----+-----+-----+ operation + result
     *  lhs | rhs | lhs | rhs |           | 
     *  ----+-----+-----+-----+-----------+-------
     *    6 |   9 |  6  |  9  |   6 < 9   | true
     *  ----+-----+-----+-----+-----------+-------+
     *   -6 |  -9 |  6  |  9  |   6 < 9   | true  | THIS WOULD BE WRONG!
     *  ----+-----+-----+-----+-----------+-------+
     *   -6 |  -9 |  9  |  6  |   9 < 6   | false | LHS AND RHS MUST BE SWAPPED!
     *
     */
    auto const negative_numbers = lhs_is_negative;
    auto const working_lhs = negative_numbers
        ? take_twos_complement(rhs)
        : lhs;
    auto const working_rhs = negative_numbers
        ? take_twos_complement(lhs)
        : rhs;

    auto i = lhs.size() - 1;
    do {
        auto const lb = working_lhs.at(i);
        auto const rb = working_rhs.at(i);

        if (rb and not lb) {
            return true;
        }

        --i;
    } while (i < lhs.size());

    return false;
}
#endif
}  // namespace viua::arithmetic::bits

#if 0
auto make_signed_limits(size_t const width)
    -> std::pair<arithmetic_type, arithmetic_type>
{
    auto neg_lim = extend(arithmetic_type{}, width, false);
    neg_lim.back() = true;

    auto pos_lim = extend(arithmetic_type{}, width, true);
    pos_lim.back() = false;

    return { neg_lim, pos_lim };
}

auto clip(arithmetic_type v) -> arithmetic_type
{
    auto const last_set_bit = std::distance(v.begin(), std::find(v.end() - 1, v.begin(), true));
    v.resize(std::max(last_set_bit, decltype(last_set_bit){ 1 }));  // do not leave the vector empty
    return v;
}

auto is_zero(arithmetic_type const v) -> bool
{
    return std::ranges::find(v, true) == v.end();
}

auto is_negative(arithmetic_type const v) -> bool
{
    if (v.empty()) {
        return false;
    }

    return v.back();
}

namespace fixed {
auto inc(arithmetic_type v) -> with_carry_type
{
    auto tmp = bits::inc(v);
    if (tmp.size() > v.size()) {
        return { true, extend(tmp, v.size()) };
    } else {
        return { false, tmp };
    }
}
auto dec(arithmetic_type v) -> with_carry_type
{
    auto tmp = bits::dec(v);
    if (tmp.size() > v.size()) {
        return { true, extend(tmp, v.size()) };
    } else {
        return { false, tmp };
    }
}

auto add(arithmetic_type const lhs, arithmetic_type const rhs) -> with_carry_type
{
    auto tmp = bits::add(lhs, rhs);
    if (tmp.size() > lhs.size()) {
        return { true, extend(tmp, lhs.size()) };
    } else {
        return { false, tmp };
    }
}
auto sub(arithmetic_type const lhs, arithmetic_type const rhs) -> with_carry_type
{
    auto tmp = bits::sub(lhs, rhs);
    if (tmp.size() > lhs.size()) {
        return { true, extend(tmp, lhs.size()) };
    } else {
        return { false, tmp };
    }
}
}

namespace saturating {
auto inc(arithmetic_type v) -> arithmetic_type
{
    auto tmp = bits::inc(v);

    if (tmp.size() > v.size()) {
        auto sat = extend(arithmetic_type{}, v.size(), true);
        sat.back() = false;
        return sat;
    }
    if (is_negative(tmp) and not is_negative(v)) {
        auto sat = extend(arithmetic_type{}, v.size(), true);
        sat.back() = false;
        return sat;
    }

    return tmp;
}
auto dec(arithmetic_type v) -> arithmetic_type
{
    auto tmp = bits::dec(v);
    if (tmp.size() > v.size()) {
        auto sat = extend(arithmetic_type{}, v.size(), false);
        sat.back() = true;
        return sat;
    }
    return tmp;
}

auto add(arithmetic_type const lhs, arithmetic_type const rhs) -> arithmetic_type
{
    auto tmp = bits::add(lhs, rhs);

    /*
     * If the result is within range than we got off easy, and can just return.
     * Otherwise, overflow was triggered and we have to deal with the operation
     * the hard way.
     */
    if (tmp.size() == lhs.size()) {
        return tmp;
    }

    auto const [negative_limit, positive_limit] = make_signed_limits(lhs.size());
    auto const clipped = extend(tmp, lhs.size());
    auto const clip_tmp_same_sign = (clipped.back() == tmp.back());

    /*
    if (is_negative(tmp)) {
        auto extended_neglim = extend(negative_limit, tmp.size());
        if (bits::lt(extended_neglim, tmp) and clip_tmp_same_sign) {
            return clipped;
        } else {
            return negative_limit;
        }
    }

    return positive_limit;
    */

    auto const lhs_is_negative = is_negative(lhs);
    auto const rhs_is_negative = is_negative(rhs);

    if ((not lhs_is_negative) and (not rhs_is_negative)) {
        /*
         * Both numbers were positive so we have a case of exceeding the
         * positive limit. Easy-peasy: just return the positive limit itself and
         * we are done.
         */
        return positive_limit;
    }
    if (lhs_is_negative and rhs_is_negative) {
        /*
         * Both numbers were negative so we have a case of exceeding the
         * negative limit. Similarly to the positive overflow case, this is easy
         * to deal with: just return the negative limit.
         */
        return negative_limit;
    }

    /*
     * In mixed-sign cases the solution is also easy: just return the clipped
     * value. Why?
     *
     * Remember that for signed twos complement numbers, the negative limit is
     * ALWAYS slightly farther away from zero than the positive limit.
     * Therefore, the following conditions are ALWAYS true (assuming infinite
     * precision):
     *
     *      abs(POS) < abs(NEG)
     *
     *      NEG < (NEG + POS) < 0 < POS
     *             ~~~~~~~~~
     *      NEG < (POS + NEG) < 0 < POS
     *
     * So, if we triggered an overflow with mixed-sign numbers we can be sure
     * that the result is ALWAYS within range; and if there are any carry or
     * borrow bits in the result, we can just cut them off to get the result we
     * actually want.
     */
    return clipped;
}
auto sub(arithmetic_type const lhs, arithmetic_type const rhs) -> arithmetic_type
{
    auto tmp = bits::sub(lhs, rhs);
    if (tmp.size() > lhs.size()) {
        auto sat = extend(arithmetic_type{}, lhs.size(), false);
        sat.back() = true;
        return sat;
    }
    return tmp;
}
}
#endif
