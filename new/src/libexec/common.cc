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

#include <stdlib.h>
#include <unistd.h>

#include <numeric>
#include <optional>
#include <print>
#include <string>

#include <viua/libexec/common.hh>
#include <viua/support/print.hh>


namespace {
// Taken from std::visit(3) page on cppreference.com
template<typename>
inline constexpr auto always_false_v = false;
}  // namespace

namespace viua {
namespace libexec {
Common_options::Common_options(
    std::string_view t)
    : tool{ t }
{}

Args::Args(
    int const ac,
    char* const av[])
    : argv{ (av + 1), (av + ac) }
{}

auto Args::get_printable(
    std::string_view const label) const -> std::optional<std::string>
{
    if (not options.contains(label)) {
        return std::nullopt;
    }

    auto const& value = options.at(label);
    return std::visit(
        [](auto&& v) -> std::string
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>) {
                return v ? "true" : "false";
            } else if constexpr (std::is_same_v<T, size_t>) {
                return std::to_string(v);
            } else if constexpr (std::is_same_v<
                                     T,
                                     std::vector<std::string_view>>) {
                return std::reduce(
                           v.begin(),
                           v.end(),
                           std::string{ "[" },
                           [](auto const acc, auto const el) -> std::string
                           { return std::format("{} {}", acc, el); })
                       + " ]";
            } else if constexpr (std::is_same_v<T,
                                                std::set<std::string_view>>) {
                return std::reduce(
                           v.begin(),
                           v.end(),
                           std::string{ "{" },
                           [](auto const acc, auto const el) -> std::string
                           { return std::format("{} {}", acc, el); })
                       + " }";
            } else if constexpr (std::is_same_v<T, std::string_view>) {
                return std::string{ v };
            } else {
                static_assert(always_false_v<T>, "impossible");
            }
        },
        value);
}

auto Args::parse_with(
    ui_type const& ui) -> std::optional<std::string_view>
{
    args.clear();
    labels.clear();
    options.clear();

    auto i          = size_t{ 0 };
    auto const save = [this, &i](std::string label,
                                 Kind const kind,
                                 std::optional<std::string_view> const value =
                                     std::nullopt) -> void
    {
        auto const saved_label =
            std::string_view{ *labels.insert(label).first };

        switch (kind) {
            using enum Args::Kind;
            case Switch:
                options[saved_label] = true;
                break;
            case Level:
                ++std::get<size_t>(options[saved_label]);
                break;
            case List:
                {
                    using T = std::vector<std::string_view>;
                    if (not options.contains(saved_label)) {
                        options[saved_label] = T{};
                    }
                    std::get<T>(options[saved_label])
                        .push_back(value.has_value() ? *value : argv.at(++i));
                    break;
                }
            case Set:
                {
                    using T = std::set<std::string_view>;
                    if (not options.contains(saved_label)) {
                        options[saved_label] = T{};
                    }
                    std::get<T>(options[saved_label])
                        .insert(value.has_value() ? *value : argv.at(++i));
                    break;
                }
            case Single:
                options[saved_label] =
                    value.has_value() ? *value : argv.at(++i);
                break;
        }
    };

    for (; i < argv.size(); ++i) {
        auto a = std::string_view{ argv.at(i) };

        if (a == "--") {
            ++i;
            break;
        }

        if (a.starts_with("--")) {
            a.remove_prefix(2);

            auto valid = false;
            for (auto const& [opt, kind] : ui) {
                auto const& [_, labels] = opt;

                auto const& canonical = *labels.begin();
                for (auto const& label : labels) {
                    if ((valid = (a == label))) {
                        save(canonical, kind);

                        /*
                         * We found the match, so let's not continue uselessly
                         * iterating over all other options.
                         */
                        break;
                    }

                    auto const maybe_with_equals = std::string{ label } + '=';
                    if ((valid = a.starts_with(maybe_with_equals))) {
                        /*
                         * The option was passed as "--foo=bar". Let's skip the
                         * "foo=" part so that our a views only the "bar" part. This
                         * can then be supplied as the value to use by the saver
                         * function.
                         */
                        a.remove_prefix(maybe_with_equals.size());

                        save(canonical, kind, a);

                        /*
                         * We found the match, so let's not continue uselessly
                         * iterating over all other options.
                         */
                        break;
                    }
                }

                if (valid) {
                    break;
                }
            }

            if (not valid) {
                return argv.at(i);
            }

            continue;
        }

        if (a.starts_with("-")) {
            a.remove_prefix(1);

            auto valid = false;
            for (auto const& [opt, kind] : ui) {
                auto const& [shortcut, labels] = opt;

                auto const& canonical = *labels.begin();
                if ((valid = (a == shortcut))) {
                    save(canonical, kind);

                    /*
                     * We found the match, so let's not continue uselessly
                     * iterating over all other options.
                     */
                    break;
                }

                auto const maybe_with_equals = std::string{ shortcut } + '=';
                if ((valid = a.starts_with(maybe_with_equals))) {
                    /*
                     * The option was passed as "-f=bar". Let's skip the "f="
                     * part so that our a views only the "bar" part. This can
                     * then be supplied as the value to use by the saver
                     * function.
                     */
                    a.remove_prefix(maybe_with_equals.size());

                    save(canonical, kind, a);

                    /*
                     * We found the match, so let's not continue uselessly
                     * iterating over all other options.
                     */
                    break;
                }
            }

            if (not valid) {
                return argv.at(i);
            }

            continue;
        }

        /*
         * Not an option, so it must be an operand. Finish iterating through
         * argv and gather whatever is left into the
         */
        break;
    }
    std::copy(argv.begin() + i, argv.end(), std::back_inserter(args));

    return std::nullopt;
}

auto parse_with_or_exit(
    Args& args,
    Args::ui_type const& ui) -> void
{
    if (auto const uo = args.parse_with(ui); uo.has_value()) {
        viua::support::errorln("unknown option: {}", *uo);
        exit(1);
    }
}

auto pass_or_exit(
    Args& args,
    std::string_view const tool,
    Args::ui_type const& ui) -> void
{
    parse_with_or_exit(args, ui);
    maybe_show_info_and_exit(tool, args);
}

auto args_or_exit(
    std::string_view const tool,
    int const ac,
    char* const av[],
    Args::ui_type const& ui) -> Args
{
    auto args = Args{ ac, av };
    pass_or_exit(args, tool, ui);
    return args;
}

auto maybe_show_info_and_exit(
    Common_options const& o) -> std::optional<int>
{
    if (o.show.version) {
        /*
         * For the explanation of why this specific version format is used
         * consult the following GNU standards page:
         * https://www.gnu.org/prep/standards/html_node/_002d_002dversion.html
         */
        std::println("{} (Viua VM) {}",
                     o.tool,
                     (o.verbosity ? VIUAVM_VERSION_FULL : VIUAVM_VERSION));
    }
    if (o.show.built_with) {
        std::println("compiler: {} {}", CXX, CXXVERSION);
        std::println("standard: {}", CXXSTD);
        std::println("preset:   {}", VIUAVM_CXX_PRESET);
        std::println("options:  {}", VIUAVM_CXX_OPTIONS);
    }
    if (o.show.version or o.show.built_with) {
        return 0;
    }

    if (o.show.help) {
        auto const man_page = std::format("viua-{}", o.tool);
        if (execlp("man", "man", "1", man_page.c_str(), nullptr) == -1) {
            viua::support::errorln(
                "man(1) page for {} not installed or not found", man_page);
            return 1;
        }
        return 0;
    }

    return std::nullopt;
}

auto maybe_show_info_and_exit(
    std::string_view const tool,
    Args const& a) -> void
{
    auto const show_version    = a.get<bool>("version").value_or(false);
    auto const show_built_with = a.get<bool>("built-with").value_or(false);
    auto const show_help       = a.get<bool>("help").value_or(false);
    auto const verbose         = a.get<bool>("verbose").value_or(false);

    if (show_version) {
        /*
         * For the explanation of why this specific version format is used
         * consult the following GNU standards page:
         * https://www.gnu.org/prep/standards/html_node/_002d_002dversion.html
         */
        std::println("{} (Viua VM) {}",
                     tool,
                     (verbose ? VIUAVM_VERSION_FULL : VIUAVM_VERSION));
    }
    if (show_built_with) {
        std::println("compiler: {} {}", CXX, CXXVERSION);
        std::println("standard: {}", CXXSTD);
        std::println("preset:   {}", VIUAVM_CXX_PRESET);
        std::println("options:  {}", VIUAVM_CXX_OPTIONS);
    }
    if (show_version or show_built_with) {
        exit(0);
    }

    if (show_help) {
        auto const man_page = std::format("viua-{}", tool);
        if (execlp("man", "man", "1", man_page.c_str(), nullptr) == -1) {
            viua::support::errorln(
                "man(1) page for {} not installed or not found", man_page);
            exit(1);
        }
        exit(0);
    }
}
}  // namespace libexec
}  // namespace viua
