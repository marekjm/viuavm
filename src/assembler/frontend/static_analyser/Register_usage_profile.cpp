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

#include <viua/assembler/frontend/static_analyser.h>

namespace viua {
namespace assembler {
namespace frontend {
namespace static_analyser {
using viua::cg::lex::InvalidSyntax;
using viua::cg::lex::Token;
using viua::cg::lex::TracedSyntaxError;

auto Register_usage_profile::fresh(Register const r) const -> bool {
    return fresh_registers.count(r);
}

auto Register_usage_profile::defresh() -> void {
    fresh_registers.clear();
}

auto Register_usage_profile::define(Register const r,
                                    Token const t,
                                    bool const allow_overwrites) -> void {
    if (defined(r) and fresh(r) and not allow_overwrites) {
        throw TracedSyntaxError{}
            .append(viua::cg::lex::UnusedValue{t, "overwrite of unused value:"})
            .append(
                InvalidSyntax{at(r).first}.note("unused value defined here:"));
    }
    defined_registers.insert_or_assign(r, std::pair<Token, Register>(t, r));
    fresh_registers.insert(r);
}
auto Register_usage_profile::defined(Register const r) const -> bool {
    return defined_registers.count(r);
}
auto Register_usage_profile::defined_where(Register const r) const -> Token {
    return defined_registers.at(r).first;
}

auto Register_usage_profile::infer(
    Register const r,
    const viua::internals::ValueTypes value_type_id,
    Token const& t) -> void {
    auto reg              = at(r);
    reg.second.value_type = value_type_id;
    reg.second.inferred   = {true, t};

    auto was_fresh = fresh(r);
    define(reg.second, reg.first);
    if (not was_fresh) {
        fresh_registers.erase(r);
    }
}

auto Register_usage_profile::at(Register const r) const -> const
    decltype(defined_registers)::mapped_type {
    return defined_registers.at(r);
}

auto Register_usage_profile::used(Register const r) const -> bool {
    return used_registers.count(r);
}
auto Register_usage_profile::use(Register const r, Token const t) -> void {
    used_registers[r] = t;
    fresh_registers.erase(r);
}

auto Register_usage_profile::erase(Register const r, Token const& token)
    -> void {
    erased_registers.emplace(r, token);
    defined_registers.erase(defined_registers.find(r));
}
auto Register_usage_profile::erased(Register const r) const -> bool {
    return (erased_registers.count(r) == 1);
}
auto Register_usage_profile::erased_where(Register const r) const -> Token {
    return erased_registers.at(r);
}

auto Register_usage_profile::begin() const
    -> decltype(defined_registers.begin()) {
    return defined_registers.begin();
}
auto Register_usage_profile::end() const -> decltype(defined_registers.end()) {
    return defined_registers.end();
}
}  // namespace static_analyser
}  // namespace frontend
}  // namespace assembler
}  // namespace viua
