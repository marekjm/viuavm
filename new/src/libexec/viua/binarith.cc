#include <print>

#include <viua/support/binarith.hh>


auto main() -> int
{
    using namespace viua::arithmetic;
    auto const sep = viua::arithmetic::DEFAULT_SEPARATOR;

    auto const to_string_or_overflow = [&sep](std::optional<signed_type> const v) -> std::string
    {
         return v.transform([&sep](auto const v) -> std::string
         {
            return to_string(v, false, sep);
         }).value_or("overflow");
    };

    if constexpr (false) {
        auto const one        = signed_type{ int8_t{ 1 } };
        auto const seven      = signed_type{ int8_t{ 7 } };
        auto const forty_two  = signed_type{ int8_t{ 42 } };
        auto const sixty_nine = signed_type{ int8_t{ 69 } };
        auto const rudy       = signed_type{ int8_t{ 102 } };
        std::println("  1 = {}", to_string(one, false, sep));
        std::println("  7 = {}", to_string(seven, false, sep));
        std::println(" 42 = {}", to_string(forty_two, false, sep));
        std::println("{:3d} = {}",
                     static_cast<int32_t>(sixty_nine),
                     to_string(sixty_nine, false, sep));
        std::println("{:3d} = {}",
                     static_cast<int64_t>(rudy),
                     to_string(rudy, false, sep));

        {
            using namespace viua::arithmetic::fixed;

            std::println();
            std::println("using style: fixed (i{})", one.size());

            auto const l = sixty_nine;
            auto const r = seven;

            auto v = l + r;
            std::println("  {:3d} + {:3d} = {:>12} ({})",
                         static_cast<int8_t>(l),
                         static_cast<int8_t>(r),
                         to_string(v, false, sep),
                         static_cast<int8_t>(v));

            v = l - r;
            std::println("  {:3d} - {:3d} = {:>12} ({})",
                         static_cast<int8_t>(l),
                         static_cast<int8_t>(r),
                         to_string(v, false, sep),
                         static_cast<int8_t>(v));
        }

        /*
         * Interesting numbers for testing:
         *
         *      zero    ie, 0
         *      one     ie, 1
         *      -one    ie, -1
         *      max     eg, 127 (the upper limit)
         *      min     eg, -128 (the lower limit)
         */
        auto const test_pairs = std::vector<std::pair<int8_t, int8_t>>{
            { 0, 0 },     // zero # zero
            { 0, 1 },     // zero # one
            { 0, -1 },    // zero # -one
            { 0, 127 },   // zero # max
            { 0, -128 },  // zero # min

            { 1, 0 },     // one # zero
            { 1, 1 },     // one # one
            { 1, -1 },    // one # -one
            { 1, 127 },   // one # max
            { 1, -128 },  // one # min

            { -1, 0 },     // -one # zero
            { -1, 1 },     // -one # one
            { -1, -1 },    // -one # -one
            { -1, 127 },   // -one # max
            { -1, -128 },  // -one # min

            { 127, 0 },     // max # zero
            { 127, 1 },     // max # one
            { 127, -1 },    // max # -one
            { 127, 127 },   // max # max
            { 127, -128 },  // max # min

            { -128, 0 },     // min # zero
            { -128, 1 },     // min # one
            { -128, -1 },    // min # -one
            { -128, 127 },   // min # max
            { -128, -128 },  // min # min
        };
        std::println();
        std::println("using style: saturating (i{})", one.size());
        auto draw_empty_line = test_pairs[0].first;

        for (auto const& [lhs, rhs] : test_pairs) {
            using namespace viua::arithmetic::trapping;

            if ((lhs != draw_empty_line) or true) {
                std::println();
                draw_empty_line = lhs;
            }

            auto const l = signed_type{ lhs };
            auto const r = signed_type{ rhs };

            auto v = l + r;
            std::println("  {:4d} + {:4d} = {:>12} ({})",
                         static_cast<int8_t>(l),
                         static_cast<int8_t>(r),
                         to_string_or_overflow(v),
                         static_cast<int8_t>(v.value_or(0)));

            v = l - r;
            std::println("  {:4d} - {:4d} = {:>12} ({})",
                         static_cast<int8_t>(l),
                         static_cast<int8_t>(r),
                         to_string_or_overflow(v),
                         static_cast<int8_t>(v.value_or(0)));
        }
    }

    /*
     * Interesting numbers for testing:
     *
     *      zero    ie, 0
     *      one     ie, 1
     *      -one    ie, -1
     *      max     eg, 127 (the upper limit)
     *      min     eg, -128 (the lower limit)
     */
    using overflow_test_type = std::tuple<bool, int8_t, int8_t>;
    constexpr auto test_pairs_ii = std::array<overflow_test_type, 29>{{
        { false, 0, 0 },     // zero # zero
        { false, 0, 1 },     // zero # one
        { false, 0, -1 },    // zero # -one
        { false, 0, 127 },   // zero # max
        { false, 0, -128 },  // zero # min

        { false, 1, 0 },     // one # zero
        { false, 1, 1 },     // one # one
        { false, 1, -1 },    // one # -one
        { false, 1, 127 },   // one # max
        { false, 1, -128 },  // one # min

        { false, -1, 0 },     // -one # zero
        { false, -1, 1 },     // -one # one
        { false, -1, -1 },    // -one # -one
        { false, -1, 127 },   // -one # max
        { true, -1, -128 },  // -one # min

        { false, 127, 0 },     // max # zero
        { false, 127, 1 },     // max # one
        { false, 127, -1 },    // max # -one
        { true, 127, 127 },   // max # max
        { true, 127, -128 },  // max # min

        { false, -128, 0 },     // min # zero
        { false, -128, 1 },     // min # one
        { true, -128, -1 },    // min # -one
        { true, -128, 127 },   // min # max
        { true, -128, -128 },  // min # min

        { true, -64, -2 },  // -half # two
        { true, -69, -3 },
        { true, -128, -3 },
        { true, 127, -2 },
    }};
    for (auto const& [expect_overflow, a, b] : test_pairs_ii) {
        auto const lhs = signed_type{ a };
        auto const rhs = signed_type{ b };

        using namespace viua::arithmetic::trapping;
        auto const v = (lhs * rhs);

        auto const test_ok = expect_overflow xor v.has_value();

        std::println("[{:^4}]  {} ({:4}) * {} ({:4}) : {}{}",
                     (test_ok ? "ok" : "fail"),
                     to_string(lhs, false, sep),
                     static_cast<int8_t>(lhs),
                     to_string(rhs, false, sep),
                     static_cast<int8_t>(rhs),
                     to_string_or_overflow(v),
                     v.transform([](auto const x) -> std::string
                     {
                        return std::format(" ({})", static_cast<int8_t>(x));
                     }).value_or(""));
    }

    return 0;
}
