/*
 *  Copyright (C) 2018 Marek Marecki
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
#include <iostream>
#include <string>
#include <vector>
#include <viua/version.h>

static auto make_args(int const argc, char const* const argv[]) -> std::vector<std::string> {
    auto args = std::vector<std::string>{};
    std::copy_n(argv, argc, std::back_inserter(args));
    return args;
}

std::string const OPTION_HELP_LONG = "--help";
std::string const OPTION_HELP_SHORT = "-h";
std::string const OPTION_VERSION = "--version";

static auto usage(std::vector<std::string> const& args) -> bool {
    auto help_screen = args.empty();
    auto version_screen = false;
    if (std::find(args.begin(), args.end(), OPTION_HELP_LONG) != args.end()) {
        help_screen = true;
    }
    if (std::find(args.begin(), args.end(), OPTION_HELP_SHORT) != args.end()) {
        help_screen = true;
    }
    if (std::find(args.begin(), args.end(), OPTION_VERSION) != args.end()) {
        version_screen = true;
    }

    if (version_screen) {
        std::cout << "Viua VM assembler version " << VERSION << '.' << MICRO << std::endl;
    }
    if (help_screen) {
        if (version_screen) {
            std::cout << '\n';
        }
        std::cout << "SYNOPSIS\n";
        std::cout << "    " << args.at(0) << " [<option>...] [-o <output>] <source>.asm [<module>...]\n";
        std::cout << '\n';
        std::cout << "    <option> is any option that is accepted by the assembler.\n";
        std::cout << "    <output> is the file to which the assembled bytecode will be written."
                     " The default is to write output to `a.out` file.\n";
        std::cout << "    <source> is the name of the file containing the Viua VM assembly language"
                     " program to be assembled.\n";
        std::cout << "    <module> is a module that is to be statically linked to the currently assembled"
                     " file.\n";

        std::cout << '\n';

        std::cout << "OPTIONS\n";
        std::cout << "    -h, --help        - display help screen\n";
        std::cout << "        --version     - display version\n";
        std::cout << "    -o, --out <file>  - write output to <file>; the default is to write to `a.out`\n";
        std::cout << "    -c, --lib         - assemble as a linkable module; the default is to assemble an"
                     " executable\n";
        std::cout << "    -C, --verify      - perform static analysis, but do not output bytecode\n";
        std::cout << "        --no-sa       - disable static analysis (useful in case of false positives"
                     " being thrown by the SA engine)\n";

        std::cout << '\n';

        std::cout << "COPYRIGHT\n";
        std::cout << "    Copyright (C) 2018 Marek Marecki\n";
        std::cout << "    License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n";
        std::cout << "    This is free software: you are free to change and redistribute it.\n";
        std::cout << "    There is NO WARRANTY, to the extent permitted by law.\n";

    }

    return (help_screen or version_screen);
}

auto main(int argc, char* argv[]) -> int {
    auto const args = make_args(argc, argv);
    if (usage(args)) {
        return 0;
    }

    return 0;
}
