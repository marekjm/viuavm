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

#include <iostream>
#include <sstream>

#include <viua/assembler/frontend/static_analyser.h>
#include <viua/support/string.h>

using viua::assembler::frontend::parser::Instruction;

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
auto check_for_unused_values(
    Register_usage_profile const& register_usage_profile) -> void
{
    if (allowed_error(Reportable_error::Unused_value)) {
        return;
    }

    for (auto const& each : register_usage_profile) {
        if (not each.first.index) {
            /*
             * Registers with index 0 do not take part in the "unused" analysis,
             * because they are used to store return values of functions. This
             * means that they *MUST* be defined, but *MAY* stay unused.
             */
            continue;
        }

        // FIXME Implement the .unused: directive, and [[maybe_unused]]
        // attribute (later).
        if (not register_usage_profile.used(each.first)) {
            auto msg = std::ostringstream{};
            msg << "unused " + to_string(each.second.second.value_type)
                       + " in register "
                << str::enquote(std::to_string(each.first.index));
            if (register_usage_profile.index_to_name.count(each.first.index)) {
                msg << " (named "
                    << str::enquote(register_usage_profile.index_to_name.at(
                           each.first.index))
                    << ')';
            }

            throw viua::cg::lex::Unused_value{each.second.first, msg.str()};
        }
    }
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
