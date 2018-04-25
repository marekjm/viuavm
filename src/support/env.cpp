/*
 *  Copyright (C) 2015, 2016, 2018 Marek Marecki
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

#include <sstream>
#include <viua/support/env.h>
using namespace std;

namespace support { namespace env {
auto get_var(string const& var) -> string {
    auto const VAR = getenv(var.c_str());
    return (VAR == nullptr ? string("") : string(VAR));
}
auto get_paths(string const& var) -> vector<string> {
    auto path  = get_var(var);
    auto paths = vector<string>{};

    auto a_path = string{};
    auto i      = decltype(path)::size_type{0};
    while (i < path.size()) {
        if (path[i] == ':') {
            if (a_path.size()) {
                paths.emplace_back(a_path);
                a_path = "";
                ++i;
            }
        }
        a_path += path[i];
        ++i;
    }
    if (a_path.size()) {
        paths.emplace_back(a_path);
    }

    return paths;
}

auto is_file(string const& path) -> bool {
    struct stat sf;

    // not a file if stat returned error
    if (stat(path.c_str(), &sf) == -1) {
        return false;
    }
    // not a file if S_ISREG() macro returned false
    if (not S_ISREG(sf.st_mode)) {
        return false;
    }

    // file otherwise
    return true;
}

namespace viua {
auto get_mod_path(string const& module,
                  string const& extension,
                  vector<string> const& paths) -> string {
    auto path  = string{""};
    auto found = false;

    auto oss = ostringstream{};
    for (auto const& each : paths) {
        oss.str("");
        oss << each << '/' << module << '.' << extension;
        path = oss.str();
        if (path[0] == '~') {
            oss.str("");
            oss << getenv("HOME") << path.substr(1);
            path = oss.str();
        }

        if ((found = support::env::is_file(path))) {
            break;
        }
    }

    return (found ? path : "");
}
}  // namespace viua
}}  // namespace support::env
