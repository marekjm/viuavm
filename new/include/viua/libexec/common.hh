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

#ifndef VIUA_LIBEXEC_COMMON_HH
#define VIUA_LIBEXEC_COMMON_HH

#include <stdint.h>

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>
#include <variant>
#include <vector>


namespace viua {
namespace libexec {
struct Common_options {
    std::string_view tool;

    size_t verbosity{ 0 };
    struct {
        bool version{ false };
        bool built_with{ false };
        bool help{ false };
    } show;

    explicit Common_options(std::string_view);
};

struct Args {
    /*
     * A more convenient way of accessing argv.
     */
    std::vector<std::string> argv;

    /*
     * Only the arguments ie, non-option elements found on the command line.
     */
    std::vector<std::string_view> args;

    /*
     * Map from --option to its value.
     */
    enum class Kind
    {
        Switch,
        Level,
        List,
        Set,
        Single,
    };
    using value_type = std::variant<
        /*
         * Used for yes-or-no switches eg, --version or --help. They have
         * meaning just due to being present on the command line.
         */
        bool,

        /*
         * Used for "countable" switches eg, --verbose where how many times an
         * option appears changes its meaning. Think
         *
         *      ]$ ssh user@example.com
         *
         * vs
         *
         *      ]$ ssh -vvv user@example.com
         *
         * which increases the verbosity level.
         */
        size_t,

        /*
         * Used for "list" switches eg, -I or -l in GCC where the user can
         * create a list of values. Think
         *
         *      ]$ gcc -I /foo/include -I /bar/include
         *
         * which adds /foo/include and /bar/include to the include search list.
         */
        std::vector<std::string_view>,

        /*
         * Same as list, except used for options where the ordering of the
         * sequence does not really matter.
         */
        std::set<std::string_view>,

        /*
         * Used for "option" switches eg, --message in Git where the user can
         * supply a single non-default value to the program.
         */
        std::string_view>;
    std::set<std::string> labels;
    std::map<std::string_view, value_type> options;

    Args(int const, char* const[]);

    auto get_printable(std::string_view const) const
        -> std::optional<std::string>;

    template<typename T>
    auto get(
        std::string_view const label) const -> std::optional<T>
    {
        if (not options.contains(label)) {
            return std::nullopt;
        }

        auto const& value = options.at(label);
        if (not std::holds_alternative<T>(value)) {
            throw std::logic_error{
                "option present, but bad type was requested"
            };
        }

        return std::get<T>(value);
    }

    template<typename T,
             typename V       = T,
             typename Present = std::function<V(T)>,
             typename Absent  = std::function<V()>>
    auto map(
        std::string_view const label,
        Present&& p,
        Absent&& a) const -> V
    {
        auto v = get<T>(label);
        if (v.has_value()) {
            return p(*v);
        } else {
            return a();
        }
    }

    using ui_type = std::map<std::tuple<std::string, std::string>, Kind>;
    auto parse_with(ui_type const&) -> std::optional<std::string_view>;
};

auto parse_with_or_exit(Args&, Args::ui_type const&) -> void;
auto pass_or_exit(Args&, std::string_view const, Args::ui_type const&) -> void;
auto args_or_exit(std::string_view const,
                  int const,
                  char* const[],
                  Args::ui_type const&) -> Args;

auto maybe_show_info_and_exit(std::string_view const tool, Args const&) -> void;
auto maybe_show_info_and_exit(Common_options const&) -> std::optional<int>;
}  // namespace libexec
}  // namespace viua

#endif
