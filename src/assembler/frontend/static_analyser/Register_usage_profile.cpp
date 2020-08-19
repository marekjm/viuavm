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

#include <assert.h>

#include <algorithm>
#include <iostream>

#include <viua/assembler/frontend/static_analyser.h>


static auto fail[[noreturn]](std::string message) -> void
{
    std::cerr << (message + "\n");
    assert(false);
}

namespace viua { namespace assembler { namespace frontend {
namespace static_analyser {
using viua::cg::lex::Invalid_syntax;
using viua::cg::lex::Token;
using viua::cg::lex::Traced_syntax_error;

auto Register_usage_profile::fresh(Register const r) const -> bool
{
    if (r.register_set == viua::bytecode::codec::Register_set::Local) {
        return fresh_registers_local.count(r);
    }
    return false;
}

auto Register_usage_profile::defresh() -> void
{
    fresh_registers_local.clear();
    fresh_registers_arguments.clear();
}
auto Register_usage_profile::defresh(Register const r) -> void
{
    if (r.register_set == viua::bytecode::codec::Register_set::Local) {
        fresh_registers_local.erase(r);
    }
    if (r.register_set == viua::bytecode::codec::Register_set::Arguments) {
        fresh_registers_arguments.erase(r);
    }
}
auto Register_usage_profile::erase_arguments(Token const tok) -> void
{
    for (auto const& each : fresh_registers_arguments) {
        erase(each, tok);
    }
    fresh_registers_arguments.clear();
}

auto Register_usage_profile::define(Register const r,
                                    Token const t,
                                    bool const allow_overwrites) -> void
{
    using viua::bytecode::codec::Register_set;
    if ((not in_bounds(r))
        and !(r.register_set == viua::bytecode::codec::Register_set::Global
              or r.register_set == viua::bytecode::codec::Register_set::Static
              or r.register_set
                     == viua::bytecode::codec::Register_set::Parameters)) {
        /*
         * Do not throw on global or static register set access.
         * There is currently no simple (or complicated) way to check if such
         * accesses are correct or not.
         * FIXME Maybe check global and static register set accesses?
         */
        throw Traced_syntax_error{}
            .append(Invalid_syntax{
                t,
                ("access to register " + std::to_string(r.index) + " with only "
                 + std::to_string(allocated_registers().value())
                 + " register(s) allocated")}
                    .add(t))
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

    if (r.register_set == viua::bytecode::codec::Register_set::Local) {
        defined_registers_local.insert_or_assign(r, std::pair<Token, Register>{t, r});
        fresh_registers_local.insert(r);
    } else if (r.register_set == viua::bytecode::codec::Register_set::Static) {
        defined_registers_static.insert_or_assign(r, std::pair<Token, Register>{t, r});
        fresh_registers_static.insert(r);
    } else if (r.register_set == viua::bytecode::codec::Register_set::Global) {
        // FIXME At least try to track global registers
    } else if (r.register_set == viua::bytecode::codec::Register_set::Arguments) {
        defined_registers_arguments.insert_or_assign(r, std::pair<Token, Register>{t, r});
        fresh_registers_arguments.insert(r);
    } else if (r.register_set == viua::bytecode::codec::Register_set::Parameters) {
        defined_registers_parameters.insert_or_assign(r, std::pair<Token, Register>{t, r});
        fresh_registers_parameters.insert(r);
    } else {
        fail(("did not expect " + to_string(r) + " register in ::define()"));
    }
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
              or r.register_set
                     == viua::bytecode::codec::Register_set::Parameters)) {
        /*
         * Do not throw on global or static register set access.
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

    if (r.register_set == viua::bytecode::codec::Register_set::Local) {
        defined_registers_local.insert_or_assign(r, std::pair<Token, Register>{index, r});
        fresh_registers_local.insert(r);
    } else if (r.register_set == viua::bytecode::codec::Register_set::Arguments) {
        defined_registers_arguments.insert_or_assign(r, std::pair<Token, Register>{index, r});
        fresh_registers_arguments.insert(r);
    } else if (r.register_set == viua::bytecode::codec::Register_set::Parameters) {
        defined_registers_parameters.insert_or_assign(r, std::pair<Token, Register>{index, r});
        fresh_registers_parameters.insert(r);
    } else {
        fail(("did not expect " + to_string(r) + " register in ::define() with register set"));
    }
}
auto Register_usage_profile::defined(Register const r) const -> bool
{
    if (r.register_set == viua::bytecode::codec::Register_set::Local) {
        return defined_registers_local.count(r);
    } else if (r.register_set == viua::bytecode::codec::Register_set::Static) {
        return defined_registers_static.count(r);
    } else if (r.register_set == viua::bytecode::codec::Register_set::Global) {
        /*
         * We cannot reliably detect if global registers are defined or not so
         * just close our eyes and hope for the best.
         */
        return true;
    } else if (r.register_set == viua::bytecode::codec::Register_set::Arguments) {
        return defined_registers_arguments.count(r);
    } else if (r.register_set == viua::bytecode::codec::Register_set::Parameters) {
        return defined_registers_parameters.count(r);
    } else {
        fail(("did not expect " + to_string(r) + " register in ::defined()"));
    }
}
auto Register_usage_profile::defined_where(Register const r) const -> Token
{
    if (r.register_set == viua::bytecode::codec::Register_set::Local) {
        return defined_registers_local.at(r).first;
    } else if (r.register_set == viua::bytecode::codec::Register_set::Arguments) {
        return defined_registers_arguments.at(r).first;
    } else {
        fail(("did not expect " + to_string(r) + " register in ::defined_where()"));
    }
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
        defresh(r);
    }
}

auto Register_usage_profile::at(Register const r) const -> const
    decltype(defined_registers_local)::mapped_type
{
    if (r.register_set == viua::bytecode::codec::Register_set::Local) {
        return defined_registers_local.at(r);
    } else if (r.register_set == viua::bytecode::codec::Register_set::Arguments) {
        return defined_registers_arguments.at(r);
    } else if (r.register_set == viua::bytecode::codec::Register_set::Parameters) {
        return defined_registers_parameters.at(r);
    } else {
        fail(("did not expect " + to_string(r) + " register in ::at()"));
    }
}

auto Register_usage_profile::used(Register const r) const -> bool
{
    if (r.register_set == viua::bytecode::codec::Register_set::Local) {
        return used_registers_local.count(r);
    } else if (r.register_set == viua::bytecode::codec::Register_set::Arguments) {
        return used_registers_arguments.count(r);
    } else {
        fail(("did not expect " + to_string(r) + " register in ::used()"));
    }
}
auto Register_usage_profile::use(Register const r, Token const t) -> void
{
    if (r.register_set == viua::bytecode::codec::Register_set::Local) {
        used_registers_local[r] = t;
    }
    if (r.register_set == viua::bytecode::codec::Register_set::Arguments) {
        used_registers_arguments[r] = t;
    }
    defresh(r);
}

auto Register_usage_profile::erase(Register const r, Token const& token) -> void
{
    if (r.register_set == viua::bytecode::codec::Register_set::Local) {
        erased_registers_local.emplace(r, token);
        defined_registers_local.erase(defined_registers_local.find(r));
    }
    if (r.register_set == viua::bytecode::codec::Register_set::Arguments) {
        erased_registers_arguments.emplace(r, token);
        defined_registers_arguments.erase(defined_registers_arguments.find(r));
    }
}
auto Register_usage_profile::erased(Register const r) const -> bool
{
    if (r.register_set == viua::bytecode::codec::Register_set::Local) {
        return erased_registers_local.count(r);
    } else if (r.register_set == viua::bytecode::codec::Register_set::Arguments) {
        return erased_registers_arguments.count(r);
    } else {
        fail(("did not expect " + to_string(r) + " register in ::erased()"));
    }
}
auto Register_usage_profile::erased_where(Register const r) const -> Token
{
    if (r.register_set == viua::bytecode::codec::Register_set::Local) {
        return erased_registers_local.at(r);
    } else if (r.register_set == viua::bytecode::codec::Register_set::Arguments) {
        return erased_registers_arguments.at(r);
    } else {
        fail(("did not expect " + to_string(r) + " register in ::erased_where()"));
    }
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

auto Register_usage_profile::begin_local() const
    -> decltype(defined_registers_local.begin())
{
    return defined_registers_local.begin();
}
auto Register_usage_profile::end_local() const -> decltype(defined_registers_local.end())
{
    return defined_registers_local.end();
}

auto Register_usage_profile::for_all_defined(std::function<void(defined_register_type)> fn) const -> void
{
    for (auto const& each : defined_registers_local) {
        fn(each);
    }
    for (auto const& each : defined_registers_arguments) {
        fn(each);
    }
}
}}}}  // namespace viua::assembler::frontend::static_analyser
