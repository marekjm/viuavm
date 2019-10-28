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

#include <initializer_list>
#include <iostream>
#include <string>
#include <viua/tooling/errors/compile_time/errors.h>
#include <viua/tooling/libs/static_analyser/static_analyser.h>

namespace viua {
    namespace tooling {
        namespace libs {
            namespace static_analyser {
auto Function_state::make_wrapper(std::unique_ptr<values::Value> v)
    -> values::Value_wrapper {
    assigned_values.push_back(std::move(v));
    return values::Value_wrapper{assigned_values.size() - 1, assigned_values};
}

auto Function_state::rename_register(
    viua::internals::types::register_index const index,
    std::string name,
    viua::tooling::libs::parser::Name_directive directive) -> void {
    if (index >= local_registers_allocated) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Register_index_outside_of_allocated_range,
                directive.token(1),
                std::to_string(iota_value)})
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Empty_error,
                local_registers_allocated_where.at(2)}
                        .add(local_registers_allocated_where.at(0))
                        .note(std::to_string(local_registers_allocated)
                              + " local register(s) allocated here")
                        .aside("increase this value to "
                               + std::to_string(iota_value + 1)
                               + " to fix this error"));
    }
    if (register_index_to_name.count(index)) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Renaming_already_named_register,
                directive.token(2)}
                        .add(directive.token(1))
                        .aside("previous name was `"
                               + register_index_to_name.at(index) + "'"))
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Empty_error,
                register_renames.at(index).token(1)}
                        .add(register_renames.at(index).token(2))
                        .note("register " + std::to_string(index)
                              + " already named here"));
    }
    if (register_name_to_index.count(name)) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(
                viua::tooling::errors::compile_time::Error{
                    viua::tooling::errors::compile_time::Compile_time_error::
                        Reusing_register_name,
                    directive.token(2)}
                    .add(directive.token(1))
                    .aside("previous index was "
                           + std::to_string(register_name_to_index.at(name))))
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Empty_error,
                register_renames.at(register_name_to_index.at(name)).token(1)}
                        .add(
                            register_renames.at(register_name_to_index.at(name))
                                .token(2))
                        .note("name `" + name + "' already used here"));
    }

    register_renames.emplace(index, directive);
    register_name_to_index[name]  = index;
    register_index_to_name[index] = name;
}

auto Function_state::iota(viua::tooling::libs::lexer::Token token)
    -> viua::internals::types::register_index {
    if (iota_value >= local_registers_allocated) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Iota_outside_of_allocated_range,
                token,
                std::to_string(iota_value)})
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Empty_error,
                local_registers_allocated_where.at(2)}
                        .add(local_registers_allocated_where.at(0))
                        .note(std::to_string(local_registers_allocated)
                              + " local register(s) allocated here")
                        .aside("increase this value to "
                               + std::to_string(iota_value + 1)
                               + " to fix this error"));
    }

    return iota_value++;
}

auto Function_state::define_register(
    viua::internals::types::register_index const index,
    viua::internals::Register_sets const register_set,
    values::Value_wrapper value,
    std::vector<viua::tooling::libs::lexer::Token> location) -> void {
    auto const address = std::make_pair(index, register_set);
    defined_registers.insert_or_assign(address, value);
    defined_where.insert_or_assign(address, std::move(location));
}

auto Function_state::defined(
    viua::internals::types::register_index const index,
    viua::internals::Register_sets const register_set) const -> bool {
    return defined_registers.count(std::make_pair(index, register_set));
}

auto Function_state::defined_at(
    viua::internals::types::register_index const index,
    viua::internals::Register_sets const register_set) const
    -> std::vector<viua::tooling::libs::lexer::Token> const& {
    return defined_where.at(std::make_pair(index, register_set));
}

auto Function_state::type_of(viua::internals::types::register_index const index,
                             viua::internals::Register_sets const register_set)
    const -> values::Value_wrapper {
    return defined_registers.at(std::make_pair(index, register_set));
}

auto Function_state::mutate_register(
    viua::internals::types::register_index const index,
    viua::internals::Register_sets const register_set,
    std::vector<viua::tooling::libs::lexer::Token> location) -> void {
    auto const address = std::make_pair(index, register_set);
    auto& mut_locations =
        mutated_where.emplace(address, decltype(mutated_where)::mapped_type{})
            .first->second;
    mut_locations.push_back(std::move(location));
}

auto Function_state::mutated(
    viua::internals::types::register_index const index,
    viua::internals::Register_sets const register_set) const -> bool {
    return mutated_where.count(std::make_pair(index, register_set));
}

auto Function_state::mutated_at(
    viua::internals::types::register_index const index,
    viua::internals::Register_sets const register_set) const
    -> decltype(mutated_where)::mapped_type const& {
    return mutated_where.at(std::make_pair(index, register_set));
}

auto Function_state::erase_register(
    viua::internals::types::register_index const index,
    viua::internals::Register_sets const register_set,
    std::vector<viua::tooling::libs::lexer::Token> location) -> void {
    auto const address = std::make_pair(index, register_set);
    erased_registers.emplace(address, defined_registers.at(address));
    defined_registers.erase(defined_registers.find(address));
    erased_where.insert_or_assign(address, std::move(location));
}

auto Function_state::erased(
    viua::internals::types::register_index const index,
    viua::internals::Register_sets const register_set) const -> bool {
    return erased_registers.count(std::make_pair(index, register_set));
}

auto Function_state::erased_at(
    viua::internals::types::register_index const index,
    viua::internals::Register_sets const register_set) const
    -> std::vector<viua::tooling::libs::lexer::Token> const& {
    return erased_where.at(std::make_pair(index, register_set));
}

auto Function_state::resolve_index(
    viua::tooling::libs::parser::Register_address const& address)
    -> viua::internals::types::register_index {
    if (address.iota) {
        return iota(address.tokens().at(1));
    }

    auto const index =
        (address.name ? register_name_to_index.at(address.tokens().at(1).str())
                      : address.index);

    if (index >= local_registers_allocated) {
        // FIXME inside ::iota() there is exactly the same code
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Register_index_outside_of_allocated_range,
                address.tokens().at(1),
                std::to_string(index)})
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::
                    Empty_error,
                local_registers_allocated_where.at(2)}
                        .add(local_registers_allocated_where.at(0))
                        .note(std::to_string(local_registers_allocated)
                              + " local register(s) allocated here")
                        .aside("increase this value to "
                               + std::to_string(index + 1)
                               + " to fix this error"));
    }

    return index;
}

auto Function_state::type_matches(
    viua::internals::types::register_index const index,
    viua::internals::Register_sets const register_set,
    std::vector<values::Value_type> const type_signature) const -> bool {
    return type_of(index, register_set).to_simple() == type_signature;
}

auto Function_state::fill_type(
    values::Value_wrapper wrapper,
    std::vector<values::Value_type> const& type_signature,
    std::vector<values::Value_type>::size_type const from) -> void {
    for (auto i = from; i < type_signature.size(); ++i) {
        using values::Value_type;
        switch (type_signature.at(i)) {
        case Value_type::Integer:
            wrapper.value(std::make_unique<values::Integer>());
            break;
        case Value_type::Float:
            wrapper.value(std::make_unique<values::Float>());
            break;
        case Value_type::Vector:
            wrapper.value(std::make_unique<values::Vector>(make_wrapper(
                std::make_unique<values::Value>(Value_type::Value))));
            wrapper = static_cast<values::Vector&>(wrapper.value()).of();
            break;
        case Value_type::String:
            wrapper.value(std::make_unique<values::String>());
            break;
        case Value_type::Text:
            wrapper.value(std::make_unique<values::Text>());
            break;
        case Value_type::Pointer:
            wrapper.value(std::make_unique<values::Pointer>(make_wrapper(
                std::make_unique<values::Value>(Value_type::Value))));
            wrapper = static_cast<values::Pointer&>(wrapper.value()).of();
            break;
        case Value_type::Boolean:
            wrapper.value(std::make_unique<values::Boolean>());
            break;
        case Value_type::Bits:
            wrapper.value(std::make_unique<values::Bits>());
            break;
        case Value_type::Closure:
            wrapper.value(std::make_unique<values::Closure>());
            break;
        case Value_type::Function:
            wrapper.value(std::make_unique<values::Function>());
            break;
        case Value_type::Atom:
            wrapper.value(std::make_unique<values::Atom>());
            break;
        case Value_type::Struct:
            wrapper.value(std::make_unique<values::Struct>());
            break;
        case Value_type::Pid:
            wrapper.value(std::make_unique<values::Pid>());
            break;
        case Value_type::Value:
        default:
            // do nothing
            break;
        }
    }
}
auto Function_state::assume_type(
    viua::internals::types::register_index const index,
    viua::internals::Register_sets const register_set,
    std::vector<values::Value_type> const type_signature) -> bool {
    values::Value_wrapper wrapper = type_of(index, register_set);

    using values::Value_type;

    if (wrapper.value().type() == Value_type::Value) {
        fill_type(wrapper, type_signature, 0);
        return true;
    }

    if ((wrapper.value().type() != Value_type::Vector)
        and (wrapper.value().type() != Value_type::Pointer)) {
        return (wrapper.value().type() == type_signature.at(0))
               or (type_signature.at(0) == Value_type::Value);
    }

    auto i = decltype(type_signature)::size_type{0};
    while ((wrapper.value().type() != Value_type::Value)
           and (i < type_signature.size())) {
        if (type_signature.at(i) == Value_type::Value) {
            return true;
        }
        if (wrapper.value().type() != type_signature.at(i)) {
            return false;
        }
        if (wrapper.value().type() == Value_type::Vector) {
            wrapper = static_cast<values::Vector const&>(wrapper.value()).of();
        } else if (wrapper.value().type() == Value_type::Pointer) {
            wrapper = static_cast<values::Pointer const&>(wrapper.value()).of();
        }
        ++i;
    }

    if (i == type_signature.size()) {
        return true;
    }

    if ((i < type_signature.size())
        and (wrapper.value().type() != Value_type::Value)) {
        return false;
    }

    fill_type(wrapper, type_signature, i);

    return true;
}

static auto to_string(viua::internals::Register_sets const rs) -> std::string {
    using viua::internals::Register_sets;
    switch (rs) {
    case Register_sets::GLOBAL:
        return "global";
    case Register_sets::LOCAL:
        return "local";
    case Register_sets::STATIC:
        return "static";
    case Register_sets::ARGUMENTS:
        return "arguments";
    case Register_sets::PARAMETERS:
        return "parameters";
    case Register_sets::CLOSURE_LOCAL:
        return "closure_local";
    default:
        return "<unknown>";
    }
}
static auto to_string(values::Value_wrapper const& value) -> std::string {
    switch (value.value().type()) {
    case values::Value_type::Value:
        return "value#" + std::to_string(value.index());
    case values::Value_type::Integer:
        return "integer#" + std::to_string(value.index());
    case values::Value_type::Float:
        return "float#" + std::to_string(value.index());
    case values::Value_type::Vector:
        return "vector#" + std::to_string(value.index()) + " of "
               + to_string(
                   static_cast<values::Vector const&>(value.value()).of());
    case values::Value_type::String:
        return "string#" + std::to_string(value.index());
    case values::Value_type::Text:
        return "text#" + std::to_string(value.index());
    case values::Value_type::Pointer:
        return "pointer#" + std::to_string(value.index()) + " to "
               + to_string(
                   static_cast<values::Pointer const&>(value.value()).of());
    case values::Value_type::Boolean:
        return "boolean#" + std::to_string(value.index());
    case values::Value_type::Bits:
        return "bits#" + std::to_string(value.index());
    case values::Value_type::Closure:
        return "closure#" + std::to_string(value.index()) + " of "
               + static_cast<values::Closure const&>(value.value()).of();
    case values::Value_type::Function:
        return "function#" + std::to_string(value.index()) + " of "
               + static_cast<values::Function const&>(value.value()).of();
    case values::Value_type::Atom: {
        auto const& atom = static_cast<values::Atom const&>(value.value());
        return "atom#" + std::to_string(value.index())
               + (atom.known() ? (" of " + atom.of()) : "");
    }
    case values::Value_type::Struct:
        return "struct#" + std::to_string(value.index());
    case values::Value_type::Pid:
        return "pid#" + std::to_string(value.index());
    default:
        return "value#" + std::to_string(value.index());
    }
}

auto Function_state::dump(std::ostream& o) const -> void {
    o << "  local registers allocated: " << local_registers_allocated
      << std::endl;
    for (auto const& each : register_index_to_name) {
        std::cout << "  register " << each.first << " named `" << each.second
                  << "'" << std::endl;
    }
    for (auto const& each : defined_registers) {
        std::cout << "  " << to_string(each.first.second) << " register "
                  << each.first.first << ": contains "
                  << to_string(each.second);
        std::cout << std::endl;
    }
}

auto Function_state::clone() const -> Function_state {
    auto function_state = Function_state{local_registers_allocated,
                                         local_registers_allocated_where};

    function_state.register_renames       = register_renames;
    function_state.register_name_to_index = register_name_to_index;
    function_state.register_index_to_name = register_index_to_name;

    function_state.iota_value = iota_value;

    function_state.assigned_values.reserve(assigned_values.size());
    for (auto const& each : assigned_values) {
        function_state.assigned_values.emplace_back(
            values::Value::clone(*each, function_state.assigned_values));
    }

    function_state.defined_registers = defined_registers;
    function_state.defined_where     = defined_where;
    function_state.mutated_where     = mutated_where;
    function_state.erased_registers  = erased_registers;
    function_state.erased_where      = erased_where;

    return function_state;
}

auto Function_state::local_capacity() const
    -> viua::internals::types::register_index {
    return local_registers_allocated;
}

Function_state::Function_state(
    viua::internals::types::register_index const limit,
    std::vector<viua::tooling::libs::lexer::Token> location)
        : local_registers_allocated{limit}
        , local_registers_allocated_where{std::move(location)} {}

Function_state::Function_state(Function_state&& that)
        : local_registers_allocated{std::move(that.local_registers_allocated)}
        , local_registers_allocated_where{std::move(
              that.local_registers_allocated_where)}
        , register_renames{std::move(that.register_renames)}
        , register_name_to_index{std::move(that.register_name_to_index)}
        , register_index_to_name{std::move(that.register_index_to_name)}
        , iota_value{std::move(that.iota_value)}
        , assigned_values{}
        , defined_registers{std::move(that.defined_registers)}
        , defined_where{std::move(that.defined_where)}
        , mutated_where{std::move(that.mutated_where)}
        , erased_registers{std::move(that.erased_registers)}
        , erased_where{std::move(that.erased_where)} {
    assigned_values.reserve(assigned_values.size());
    for (auto const& each : that.assigned_values) {
        assigned_values.emplace_back(
            values::Value::clone(*each, assigned_values));
    }
}
}}}}  // namespace viua::tooling::libs::static_analyser
