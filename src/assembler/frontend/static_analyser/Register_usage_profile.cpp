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
#include <viua/assembler/frontend/static_analyser.h>

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser {
using viua::cg::lex::Invalid_syntax;
using viua::cg::lex::Token;
using viua::cg::lex::Traced_syntax_error;

auto Register_usage_profile::fresh(Register const r) const -> bool
{
    return fresh_registers.count(r);
}

auto Register_usage_profile::defresh() -> void { fresh_registers.clear(); }
auto Register_usage_profile::erase_arguments(Token const t) -> void
{
    auto args_regs = std::vector<Register>{};
    std::copy_if(fresh_registers.begin(),
                 fresh_registers.end(),
                 std::back_inserter(args_regs),
                 [](Register const& r) -> bool {
                     return r.register_set
                            == viua::bytecode::codec::Register_set::Arguments;
                 });
    for (auto const& each : args_regs) {
        erase(each, t);
        fresh_registers.erase(fresh_registers.find(each));
    }
}

auto Register_usage_profile::define(Register const r,
                                    Token const t,
                                    bool const allow_overwrites) -> void
{
    using viua::bytecode::codec::Register_set;
    if ((not in_bounds(r))
        and !(r.register_set == viua::bytecode::codec::Register_set::Global
              or r.register_set == viua::bytecode::codec::Register_set::Static
              or r.register_set == viua::bytecode::codec::Register_set::Parameters)) {
        /*
         * Do not thrown on global or static register set access.
         * There is currently no simple (or complicated) way to check if such
         * accesses are correct or not.
         * FIXME Maybe check global and static register set accesses?
         */
        throw Traced_syntax_error{}
            .append(Invalid_syntax{
                t,
                ("access to register " + std::to_string(r.index) + " with only "
                 + std::to_string(allocated_registers().value())
                 + " register(s) allocated")})
            .append(Invalid_syntax{allocated_where().value(), ""}.note(
                "increase this value to " + std::to_string(r.index + 1)
                + " to fix this issue"));
    }
    if (defined(r) and fresh(r) and not allow_overwrites) {
        throw Traced_syntax_error{}
            .append(
                viua::cg::lex::Unused_value{t, "overwrite of unused value:"})
            .append(
                Invalid_syntax{at(r).first}.note("unused value defined here:"));
    }
    defined_registers.insert_or_assign(r, std::pair<Token, Register>(t, r));
    fresh_registers.insert(r);
}
auto Register_usage_profile::define(Register const r,
                                    Token const index,
                                    Token const register_set,
                                    bool const allow_overwrites) -> void
{
    using viua::bytecode::codec::Register_set;
    if ((not in_bounds(r))
        and !(r.register_set == viua::bytecode::codec::Register_set::Global
              or r.register_set == viua::bytecode::codec::Register_set::Static
              or r.register_set == viua::bytecode::codec::Register_set::Parameters)) {
        /*
         * Do not thrown on global or static register set access.
         * There is currently no simple (or complicated) way to check if such
         * accesses are correct or not.
         * FIXME Maybe check global and static register set accesses?
         */
        throw Traced_syntax_error{}
            .append(Invalid_syntax{
                index,
                ("access to register " + std::to_string(r.index) + " with only "
                 + std::to_string(allocated_registers().value())
                 + " register(s) allocated")}
                        .add(register_set))
            .append(Invalid_syntax{allocated_where().value(), ""}.note(
                "increase this value to " + std::to_string(r.index + 1)
                + " to fix this issue"));
    }
    if (defined(r) and fresh(r) and not allow_overwrites) {
        throw Traced_syntax_error{}
            .append(viua::cg::lex::Unused_value{index,
                                                "overwrite of unused value:"})
            .append(
                Invalid_syntax{at(r).first}.note("unused value defined here:"));
    }
    defined_registers.insert_or_assign(r, std::pair<Token, Register>(index, r));
    fresh_registers.insert(r);
}
auto Register_usage_profile::defined(Register const r) const -> bool
{
    return defined_registers.count(r);
}
auto Register_usage_profile::defined_where(Register const r) const -> Token
{
    return defined_registers.at(r).first;
}

auto Register_usage_profile::infer(
    Register const r,
    const viua::internals::Value_types value_type_id,
    Token const& t) -> void
{
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
    decltype(defined_registers)::mapped_type
{
    return defined_registers.at(r);
}

auto Register_usage_profile::used(Register const r) const -> bool
{
    return used_registers.count(r);
}
auto Register_usage_profile::use(Register const r, Token const t) -> void
{
    used_registers[r] = t;
    fresh_registers.erase(r);
}

auto Register_usage_profile::erase(Register const r, Token const& token) -> void
{
    erased_registers.emplace(r, token);
    defined_registers.erase(defined_registers.find(r));
}
auto Register_usage_profile::erased(Register const r) const -> bool
{
    return (erased_registers.count(r) == 1);
}
auto Register_usage_profile::erased_where(Register const r) const -> Token
{
    return erased_registers.at(r);
}

auto Register_usage_profile::allocated_registers(
    viua::bytecode::codec::register_index_type const n) -> void
{
    no_of_allocated_registers = n;
}
auto Register_usage_profile::allocated_registers() const
    -> std::optional<viua::bytecode::codec::register_index_type>
{
    return no_of_allocated_registers;
}
auto Register_usage_profile::allocated_where(viua::cg::lex::Token const& token)
    -> void
{
    where_registers_were_allocated = token;
}
auto Register_usage_profile::allocated_where() const
    -> std::optional<viua::cg::lex::Token>
{
    return where_registers_were_allocated;
}

auto Register_usage_profile::in_bounds(Register const r) const -> bool
{
    // FIXME check in bound-ness of register sets other than local
    return not(allocated_registers()
               and r.index >= allocated_registers().value());
}

auto Register_usage_profile::begin() const
    -> decltype(defined_registers.begin())
{
    return defined_registers.begin();
}
auto Register_usage_profile::end() const -> decltype(defined_registers.end())
{
    return defined_registers.end();
}
}}}}  // namespace viua::assembler::frontend::static_analyser
