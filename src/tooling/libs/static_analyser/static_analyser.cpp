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

#include <string>
#include <viua/tooling/errors/compile_time/errors.h>
#include <viua/tooling/libs/static_analyser/static_analyser.h>

namespace viua {
namespace tooling {
namespace libs {
namespace static_analyser {

namespace values {
Value::Value(Value_type const t):
    type_of_value{t}
{}

auto Value::type() const -> Value_type {
    return type_of_value;
}

Integer::Integer(): Value{Value_type::Integer} {}

Vector::Vector(std::unique_ptr<Value> v):
    Value{Value_type::Vector}
    , contained_type{std::move(v)}
{}

auto Vector::of() const -> std::unique_ptr<Value> const& {
    return contained_type;
}
auto Vector::of(std::unique_ptr<Value> v) -> void {
    contained_type = std::move(v);
}

String::String(): Value{Value_type::String} {}
}

Function_state::Value_wrapper::Value_wrapper(index_type const v, map_type const& m):
    i{v}
    , values{m}
{}

Function_state::Value_wrapper::Value_wrapper(Value_wrapper const& that):
    i{that.i}
    , values{that.values}
{}

auto Function_state::Value_wrapper::Value_wrapper::value() const -> values::Value& {
    return *values.at(i);
}

auto Function_state::make_wrapper(std::unique_ptr<values::Value> v) -> Value_wrapper {
    assigned_values.push_back(std::move(v));
    return Value_wrapper{assigned_values.size() - 1, assigned_values};
}

auto Function_state::rename_register(
    viua::internals::types::register_index const index
    , std::string name
    , viua::tooling::libs::parser::Name_directive directive
) -> void {
    if (index >= local_registers_allocated) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Register_index_outside_of_allocated_range
                , directive.token(1)
                , std::to_string(iota_value)
            })
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Empty_error
                , local_registers_allocated_where.at(2)
            }
            .add(local_registers_allocated_where.at(0))
            .note(std::to_string(local_registers_allocated) + " local register(s) allocated here")
            .aside("increase this value to " + std::to_string(iota_value + 1) + " to fix this error"))
            ;
    }
    if (register_index_to_name.count(index)) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Renaming_already_named_register
                , directive.token(2)
            }.add(directive.token(1))
            .aside("previous name was `" + register_index_to_name.at(index) + "'")
            )
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Empty_error
                , register_renames.at(index).token(1)
            }
            .add(register_renames.at(index).token(2))
            .note("register " + std::to_string(index) + " already named here"))
            ;
    }
    if (register_name_to_index.count(name)) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Reusing_register_name
                , directive.token(2)
            }.add(directive.token(1))
            .aside("previous index was " + std::to_string(register_name_to_index.at(name)))
            )
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Empty_error
                , register_renames.at(register_name_to_index.at(name)).token(1)
            }
            .add(register_renames.at(register_name_to_index.at(name)).token(2))
            .note("name `" + name + "' already used here"))
            ;
    }

    register_renames.emplace(index, directive);
    register_name_to_index[name] = index;
    register_index_to_name[index] = name;
}

auto Function_state::iota(viua::tooling::libs::lexer::Token token) -> viua::internals::types::register_index {
    if (iota_value >= local_registers_allocated) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Iota_outside_of_allocated_range
                , token
                , std::to_string(iota_value)
            })
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Empty_error
                , local_registers_allocated_where.at(2)
            }
            .add(local_registers_allocated_where.at(0))
            .note(std::to_string(local_registers_allocated) + " local register(s) allocated here")
            .aside("increase this value to " + std::to_string(iota_value + 1) + " to fix this error"))
            ;
    }

    return iota_value++;
}

auto Function_state::define_register(
    viua::internals::types::register_index const index
    , viua::internals::Register_sets const register_set
    , Value_wrapper value
    , std::vector<viua::tooling::libs::lexer::Token> location
) -> void {
    auto const address = std::make_pair(index, register_set);
    defined_registers.emplace(address, value);
    defined_where.emplace(address, std::move(location));
}

auto Function_state::defined(
    viua::internals::types::register_index const index
    , viua::internals::Register_sets const register_set
) const -> bool {
    return defined_registers.count(std::make_pair(index, register_set));
}

auto Function_state::type_of(
    viua::internals::types::register_index const index
    , viua::internals::Register_sets const register_set
) const -> Value_wrapper {
    return defined_registers.at(std::make_pair(index, register_set));
}

auto Function_state::resolve_index(viua::tooling::libs::parser::Register_address const& address)
    -> viua::internals::types::register_index {
    if (address.iota) {
        return iota(address.tokens().at(1));
    }
    if (address.name) {
        return register_name_to_index.at(address.tokens().at(1).str());
    }
    return address.index;
}

Function_state::Function_state(
    viua::internals::types::register_index const limit
    , std::vector<viua::tooling::libs::lexer::Token> location
):
    local_registers_allocated{limit}
    , local_registers_allocated_where{std::move(location)}
{}

static auto analyse_single_function(
    viua::tooling::libs::parser::Cooked_function const& fn
    , viua::tooling::libs::parser::Cooked_fragments const&
    , Analyser_state&
) -> void {
    using viua::tooling::libs::parser::Fragment_type;
    using viua::tooling::libs::parser::Instruction;
    if (auto const& l = *fn.body().at(0); not (l.type() == Fragment_type::Instruction
                and static_cast<Instruction const&>(l).opcode == ALLOCATE_REGISTERS)) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Must_allocate_registers_first
                , l.token(0)
            }.note("the first instruction of every function must be `allocate_registers'"));
    }

    auto const& body = fn.body();
    using viua::tooling::libs::parser::Register_address;
    auto function_state[[maybe_unused]] = Function_state{
        static_cast<Register_address const*>(
            static_cast<Instruction const*>(body.at(0))->operands.at(0).get()
        )->index
        , body.at(0)->tokens()
    };

    if (fn.head().function_name == "main") {
        if (fn.head().arity == 0) {
            // no arguments passed
        } else if (fn.head().arity == 1) {
            function_state.define_register(
                0
                , viua::internals::Register_sets::PARAMETERS
                , function_state.make_wrapper(std::make_unique<values::Vector>(
                    std::make_unique<values::String>()
                ))
                , fn.head().tokens()
            );
        } else if (fn.head().arity == 2) {
            function_state.define_register(
                0
                , viua::internals::Register_sets::PARAMETERS
                , function_state.make_wrapper(std::make_unique<values::String>())
                , fn.head().tokens()
            );
            function_state.define_register(
                1
                , viua::internals::Register_sets::PARAMETERS
                , function_state.make_wrapper(std::make_unique<values::Vector>(
                    std::make_unique<values::String>()
                ))
                , fn.head().tokens()
            );
        } else {
            // FIXME arity not supported
        }
    }

    using body_size_type = std::remove_reference_t<decltype(body)>::size_type;
    for (auto i = body_size_type{1}; i < body.size(); ++i) {
        auto const line = body.at(i);
        if (line->type() == Fragment_type::Name_directive) {
            using viua::tooling::libs::parser::Name_directive;
            auto const& directive = *static_cast<Name_directive const*>(line);
            auto const index = (
                directive.iota
                ? function_state.iota(directive.token(1))
                : directive.register_index
            );

            function_state.rename_register(
                index
                , directive.name
                , directive
            );
        }

        if (line->type() == Fragment_type::Instruction) {
            auto const& instruction = *static_cast<Instruction const*>(line);

            if (instruction.opcode == INTEGER) {
                auto const& dest = *static_cast<Register_address const*>(instruction.operands.at(0).get());

                auto defining_tokens = std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));

                std::copy(dest.tokens().begin(), dest.tokens().end(), std::back_inserter(defining_tokens));

                using viua::tooling::libs::parser::Integer_literal;
                function_state.define_register(
                    function_state.resolve_index(dest)
                    , dest.register_set
                    , function_state.make_wrapper(std::make_unique<values::Integer>(
                        /* static_cast<Integer_literal const*>(instruction.operands.at(1).get())->n */
                    ))
                    , std::move(defining_tokens)
                );
            } else if (instruction.opcode == MOVE) {
                auto const& source = *static_cast<Register_address const*>(instruction.operands.at(1).get());

                auto const source_index = function_state.resolve_index(source);
                if (not function_state.defined(source_index, source.register_set)) {
                    throw viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::Compile_time_error::Access_to_empty_register
                            , source.tokens().at(1)
                        });
                }

                auto const& dest = *static_cast<Register_address const*>(instruction.operands.at(0).get());

                auto defining_tokens = std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                std::copy(dest.tokens().begin(), dest.tokens().end(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index
                    , dest.register_set
                    , function_state.type_of(source_index, source.register_set)
                    , std::move(defining_tokens)
                );
            }
        }

        function_state.dump(std::cout);
    }
}

static auto analyse_functions(
    viua::tooling::libs::parser::Cooked_fragments const& fragments
    , Analyser_state& analyser_state
) -> void {
    auto const& functions = fragments.function_fragments;

    for (auto const& [ name, fn ] : functions) {
        try {
            analyse_single_function(fn, fragments, analyser_state);
        } catch (viua::tooling::errors::compile_time::Error_wrapper& e) {
            e.append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Empty_error
                , fn.head().token(0)
                , "in function `" + name + "'"
            });
            throw;
        }
    }
}

auto analyse(viua::tooling::libs::parser::Cooked_fragments const& fragments) -> void {
    auto analyser_state = Analyser_state{};

    analyse_functions(fragments, analyser_state);

    /* analyse_closure_instantiations(fragments); */
}

}}}}
