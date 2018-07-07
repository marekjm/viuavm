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

#include <sstream>
#include <viua/assembler/frontend/static_analyser.h>
#include <viua/support/string.h>

using viua::assembler::frontend::parser::Instruction;

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser { namespace checkers {
auto check_for_unused_registers(
    Register_usage_profile const& register_usage_profile) -> void {
    const auto& limit = register_usage_profile.allocated_registers();
    if (limit.value() == 0) {
        return;
    }
    for (auto i = limit.value() - 1; i > viua::internals::types::register_index{0}; --i) {
        auto index = viua::assembler::frontend::parser::Register_index{};
        index.index = i;
        index.rss = viua::internals::Register_sets::LOCAL;
        auto const slot = viua::assembler::frontend::static_analyser::Register{index};

        if ((not register_usage_profile.defined(slot)) and (not register_usage_profile.used(slot))) {
            throw viua::cg::lex::Unused_register{
                register_usage_profile.allocated_where().value(),
                "unused local register " + std::to_string(i)};
        }
    }
}
}}}}}  // namespace viua::assembler::frontend::static_analyser::checkers
