/*
 *  Copyright (C) 2015-2017, 2020 Marek Marecki
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
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <viua/assert.h>
#include <viua/exceptions.h>
#include <viua/support/string.h>
#include <viua/types/boolean.h>
#include <viua/types/pointer.h>
#include <viua/types/string.h>
#include <viua/types/struct.h>
#include <viua/types/value.h>
#include <viua/types/vector.h>
using namespace viua::assertions;
using namespace viua::types;

std::string const viua::types::String::type_name = "String";

std::string String::type() const
{
    return type_name;
}
std::string String::str() const
{
    return svalue;
}
std::string String::repr() const
{
    return "b" + str::enquote(svalue);
}
bool String::boolean() const
{
    return svalue.size() != 0;
}

std::unique_ptr<Value> String::copy() const
{
    return std::make_unique<String>(svalue);
}

auto viua::types::String::operator==(viua::types::String const& other) const
    -> bool
{
    return (svalue == other.svalue);
}

auto String::value() const -> std::string const&
{
    return svalue;
}

// foreign methods
void String::format(Frame* frame,
                    viua::kernel::Register_set*,
                    viua::kernel::Register_set*,
                    viua::process::Process*,
                    viua::kernel::Kernel*)
{
    std::regex key_regex("#\\{(?:(?:0|[1-9][0-9]*)|[a-zA-Z_][a-zA-Z0-9_]*)\\}");

    std::string result = svalue;

    if (std::regex_search(result, key_regex)) {
        auto matches = std::vector<std::string>{};
        for (auto match =
                 std::sregex_iterator(result.begin(), result.end(), key_regex);
             match != std::sregex_iterator();
             ++match) {
            matches.emplace_back(match->str());
        }

        for (auto i : matches) {
            std::string m    = i.substr(2, (i.size() - 3));
            auto replacement = std::string{};
            bool is_number   = true;
            int index        = -1;
            try {
                index = stoi(m);
            } catch (std::invalid_argument const&) {
                is_number = false;
            }
            if (is_number) {
                replacement = static_cast<Vector*>(frame->arguments->at(1))
                                  ->at(index)
                                  ->str();
            } else {
                replacement =
                    static_cast<Struct*>(frame->arguments->at(2))->at(m)->str();
            }
            std::string pat("#\\{" + m + "\\}");
            std::regex subst(pat);
            result = std::regex_replace(result, subst, replacement);
        }
    }

    frame->local_register_set->set(0, std::make_unique<String>(result));
}

String::String(std::string s) : svalue(s)
{}

auto String::make(std::string s) -> std::unique_ptr<String>
{
    return std::make_unique<String>(std::move(s));
}
