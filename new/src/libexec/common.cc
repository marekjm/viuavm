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
            } else if constexpr (std::is_same_v<T, std::string_view>) {
                return std::string{ v };
            } else {
                static_assert(always_false_v<T>, "impossible");
            }
        },
        value);
}

auto maybe_show_info_and_exit(
    Common_options const& o) -> std::optional<int>
{
    if (o.show.version) {
        std::println("viua {} {}",
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
    Args const& a) -> std::optional<int>
{
    auto const show_version    = a.get<bool>("version").value_or(false);
    auto const show_built_with = a.get<bool>("built-with").value_or(false);
    auto const show_help       = a.get<bool>("help").value_or(false);
    auto const verbose         = a.get<bool>("verbose").value_or(false);

    if (show_version) {
        std::println("viua {} {}",
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
        return 0;
    }

    if (show_help) {
        auto const man_page = std::format("viua-{}", tool);
        if (execlp("man", "man", "1", man_page.c_str(), nullptr) == -1) {
            viua::support::errorln(
                "man(1) page for {} not installed or not found", man_page);
            return 1;
        }
        return 0;
    }

    return std::nullopt;
}
}  // namespace libexec
}  // namespace viua
