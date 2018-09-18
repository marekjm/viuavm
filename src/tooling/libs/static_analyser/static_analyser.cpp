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

Float::Float(): Value{Value_type::Float} {}

Vector::Vector(Value_wrapper v):
    Value{Value_type::Vector}
    , contained_type{v}
{}

auto Vector::of() const -> Value_wrapper const& {
    return contained_type;
}
auto Vector::of(Value_wrapper v) -> void {
    contained_type = v;
}

Pointer::Pointer(Value_wrapper v):
    Value{Value_type::Pointer}
    , contained_type{v}
{}

auto Pointer::of() const -> Value_wrapper const& {
    return contained_type;
}
auto Pointer::of(Value_wrapper v) -> void {
    contained_type = v;
}

String::String(): Value{Value_type::String} {}

Text::Text(): Value{Value_type::Text} {}

Value_wrapper::Value_wrapper(index_type const v, map_type& m):
    i{v}
    , values{&m}
{}

Value_wrapper::Value_wrapper(Value_wrapper const& that):
    i{that.i}
    , values{that.values}
{}

auto Value_wrapper::operator=(Value_wrapper const& that) -> Value_wrapper& {
    i = that.i;
    values = that.values;
    return *this;
}
auto Value_wrapper::operator=(Value_wrapper&& that) -> Value_wrapper& {
    i = that.i;
    values = that.values;
    return *this;
}

Value_wrapper::~Value_wrapper() {}

auto Value_wrapper::Value_wrapper::value() const -> Value& {
    return *(values->at(i));
}

auto Value_wrapper::Value_wrapper::value(std::unique_ptr<Value> v) -> void {
    values->at(i) = std::move(v);
}

auto Value_wrapper::to_simple() const -> std::vector<Value_type> {
    auto wrappers = std::vector<Value_wrapper const*>{ this };

    while ((wrappers.back()->value().type() == Value_type::Vector)
           or (wrappers.back()->value().type() == Value_type::Pointer)) {
        if (wrappers.back()->value().type() == Value_type::Vector) {
            wrappers.push_back(&static_cast<Vector const&>(wrappers.back()->value()).of());
        } else {
            wrappers.push_back(&static_cast<Pointer const&>(wrappers.back()->value()).of());
        }
    }

    auto simple = std::vector<Value_type>{};

    for (auto const each : wrappers) {
        simple.push_back(each->value().type());
    }

    return simple;
}
} // namespace values

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

static auto to_string(values::Value const& value) -> std::string {
    switch (value.type()) {
        case values::Value_type::Value:
            return "value";
        case values::Value_type::Integer:
            return "integer";
        case values::Value_type::Float:
            return "float";
        case values::Value_type::Vector:
            return "vector of " + to_string(static_cast<values::Vector const&>(value).of().value());
        case values::Value_type::String:
            return "string";
        case values::Value_type::Text:
            return "text";
        case values::Value_type::Pointer:
            return "pointer to " + to_string(static_cast<values::Pointer const&>(value).of().value());
        default:
            return "value";
    }
}

static auto to_string(
    std::vector<values::Value_type> const& value
    , std::vector<values::Value_type>::size_type const i
) -> std::string {
    switch (value.at(i)) {
        case values::Value_type::Value:
            return "value";
        case values::Value_type::Integer:
            return "integer";
        case values::Value_type::Float:
            return "float";
        case values::Value_type::Vector:
            return "vector of " + to_string(value, i + 1);
        case values::Value_type::String:
            return "string";
        case values::Value_type::Text:
            return "text";
        case values::Value_type::Pointer:
            return "pointer to " + to_string(value, i + 1);
        default:
            return "value";
    }
}
static auto to_string(std::vector<values::Value_type> const& value) -> std::string {
    return to_string(value, 0);
}
static auto to_string[[maybe_unused]](values::Value_type const v) -> std::string {
    switch (v) {
        case values::Value_type::Integer:
            return "integer";
        case values::Value_type::Float:
            return "float";
        case values::Value_type::Vector:
            return "vector of ...";
        case values::Value_type::String:
            return "string";
        case values::Value_type::Text:
            return "text";
        case values::Value_type::Pointer:
            return "pointer to ...";
        case values::Value_type::Value:
        default:
            return "value";
    }
}

auto Function_state::make_wrapper(std::unique_ptr<values::Value> v) -> values::Value_wrapper {
    assigned_values.push_back(std::move(v));
    return values::Value_wrapper{assigned_values.size() - 1, assigned_values};
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
    , values::Value_wrapper value
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

auto Function_state::defined_at(
    viua::internals::types::register_index const index
    , viua::internals::Register_sets const register_set
) const -> std::vector<viua::tooling::libs::lexer::Token> const& {
    return defined_where.at(std::make_pair(index, register_set));
}

auto Function_state::type_of(
    viua::internals::types::register_index const index
    , viua::internals::Register_sets const register_set
) const -> values::Value_wrapper {
    return defined_registers.at(std::make_pair(index, register_set));
}

auto Function_state::erase_register(
    viua::internals::types::register_index const index
    , viua::internals::Register_sets const register_set
    , std::vector<viua::tooling::libs::lexer::Token> location
) -> void {
    auto const address = std::make_pair(index, register_set);
    erased_registers.emplace(address, defined_registers.at(address));
    defined_registers.erase(defined_registers.find(address));
    erased_where.emplace(address, std::move(location));
}

auto Function_state::erased(
    viua::internals::types::register_index const index
    , viua::internals::Register_sets const register_set
) const -> bool {
    return erased_registers.count(std::make_pair(index, register_set));
}

auto Function_state::erased_at(
    viua::internals::types::register_index const index
    , viua::internals::Register_sets const register_set
) const -> std::vector<viua::tooling::libs::lexer::Token> const& {
    return erased_where.at(std::make_pair(index, register_set));
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

auto Function_state::type_matches(
    viua::internals::types::register_index const index
    , viua::internals::Register_sets const register_set
    , std::vector<values::Value_type> const type_signature
) const -> bool {
    return type_of(index, register_set).to_simple() == type_signature;
}

auto Function_state::assume_type(
    viua::internals::types::register_index const index
    , viua::internals::Register_sets const register_set
    , std::vector<values::Value_type> const type_signature
) -> bool {
    values::Value_wrapper wrapper = type_of(index, register_set);

    using values::Value_type;

    if ((wrapper.value().type() != Value_type::Vector) and (wrapper.value().type() != Value_type::Pointer)) {
        return wrapper.value().type() == type_signature.at(0);
    }

    auto i = decltype(type_signature)::size_type{0};
    while ((wrapper.value().type() != Value_type::Value) and (i < type_signature.size())) {
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

    if ((i < type_signature.size()) and (wrapper.value().type() != Value_type::Value)) {
        return false;
    }

    for (;i < type_signature.size(); ++i) {
        switch (type_signature.at(i)) {
            case Value_type::Integer:
                wrapper.value(std::make_unique<values::Integer>());
                break;
            case Value_type::Float:
                wrapper.value(std::make_unique<values::Float>());
                break;
            case Value_type::Vector:
                wrapper.value(std::make_unique<values::Vector>(
                    make_wrapper(std::make_unique<values::Value>(Value_type::Value))
                ));
                wrapper = static_cast<values::Vector const&>(wrapper.value()).of();
                break;
            case Value_type::String:
                break;
            case Value_type::Text:
                break;
            case Value_type::Pointer:
                break;
            case Value_type::Value:
            default:
                return true;
        }
    }

    return true;
}

auto Function_state::dump(std::ostream& o) const -> void {
    o << "  local registers allocated: " << local_registers_allocated << std::endl;
    for (auto const& each : register_index_to_name) {
        std::cout << "  register " << each.first << " named `" << each.second << "'" << std::endl;
    }
    for (auto const& each : defined_registers) {
        std::cout << "  " << to_string(each.first.second) << " register " << each.first.first << ": contains "
            << to_string(each.second.value()) << std::endl;
    }
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
    if (fn.body().size() == 0) {
        throw viua::tooling::errors::compile_time::Error_wrapper{}
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Empty_function_body
                , fn.head().token(fn.head().tokens().size() - 3)
                , "of " + fn.head().function_name
            })
            .append(viua::tooling::errors::compile_time::Error{
                viua::tooling::errors::compile_time::Compile_time_error::Empty_error
                , fn.head().token(fn.head().tokens().size() - 3)
            }.note("a function body must be composed of at least two instructions:")
             .comment("    allocate_registers %0 local")
             .comment("    return")
            );
    }

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
    auto function_state = Function_state{
        static_cast<Register_address const*>(
            static_cast<Instruction const*>(body.at(0))->operands.at(0).get()
        )->index
        , body.at(0)->tokens()
    };

    std::cout << "analyse_single_function(): " << fn.head().function_name;
    std::cout << " (" << fn.body().size() << " lines)";
    std::cout << std::endl;

    if (fn.head().function_name == "main") {
        if (fn.head().arity == 0) {
            // no arguments passed
        } else if (fn.head().arity == 1) {
            function_state.define_register(
                0
                , viua::internals::Register_sets::PARAMETERS
                , function_state.make_wrapper(std::make_unique<values::Vector>(
                    function_state.make_wrapper(std::make_unique<values::String>())
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
                    function_state.make_wrapper(std::make_unique<values::String>())
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

        std::cout << "analysing: " << line->token(0).str() << std::endl;

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

            if (instruction.opcode == NOP) {
                // do nothing
            } else if (instruction.opcode == IZERO) {
                auto const& dest = *static_cast<Register_address const*>(instruction.operands.at(0).get());

                auto defining_tokens = std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));

                std::copy(dest.tokens().begin(), dest.tokens().end(), std::back_inserter(defining_tokens));

                function_state.define_register(
                    function_state.resolve_index(dest)
                    , dest.register_set
                    , function_state.make_wrapper(std::make_unique<values::Integer>(
                        /* 0 */ // FIXME save the value in the integer
                    ))
                    , std::move(defining_tokens)
                );
            } else if (instruction.opcode == INTEGER) {
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
            } else if (instruction.opcode == FLOAT) {
                auto const& dest = *static_cast<Register_address const*>(instruction.operands.at(0).get());

                auto defining_tokens = std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));

                std::copy(dest.tokens().begin(), dest.tokens().end(), std::back_inserter(defining_tokens));

                using viua::tooling::libs::parser::Integer_literal;
                function_state.define_register(
                    function_state.resolve_index(dest)
                    , dest.register_set
                    , function_state.make_wrapper(std::make_unique<values::Float>(
                        /* static_cast<Integer_literal const*>(instruction.operands.at(1).get())->n */
                    ))
                    , std::move(defining_tokens)
                );
            } else if (instruction.opcode == MOVE) {
                auto const& source = *static_cast<Register_address const*>(instruction.operands.at(1).get());

                auto const source_index = function_state.resolve_index(source);
                if (not function_state.defined(source_index, source.register_set)) {
                    auto error = viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::Compile_time_error::Access_to_empty_register
                            , source.tokens().at(1)
                        }.add(source.tokens().at(2)));
                    if (function_state.erased(source_index, source.register_set)) {
                        auto const& erased_location =
                            function_state.erased_at(source_index, source.register_set);
                        auto partial = viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::Compile_time_error::Empty_error
                            , erased_location.at(0)
                        }.note("erased here");
                        for (auto each = erased_location.begin() + 1; each != erased_location.end(); ++each) {
                            partial.add(*each);
                        }
                        error.append(partial);
                    }
                    throw error;
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
                function_state.erase_register(
                    source_index
                    , source.register_set
                    , [&instruction, &source]() -> std::vector<viua::tooling::libs::lexer::Token> {
                        auto tokens = std::vector<viua::tooling::libs::lexer::Token>{};
                        tokens.push_back(instruction.tokens().at(0));
                        std::copy(source.tokens().begin(), source.tokens().end(), std::back_inserter(tokens));
                        return tokens;
                    }()
                );
            } else if (instruction.opcode == VAT) {
                auto const& source = *static_cast<Register_address const*>(instruction.operands.at(1).get());
                auto const& index = *static_cast<Register_address const*>(instruction.operands.at(2).get());

                auto const source_index = function_state.resolve_index(source);
                if (not function_state.defined(source_index, source.register_set)) {
                    auto error = viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::Compile_time_error::Access_to_empty_register
                            , source.tokens().at(1)
                        }.add(source.tokens().at(2)));
                    if (function_state.erased(source_index, source.register_set)) {
                        auto const& erased_location =
                            function_state.erased_at(source_index, source.register_set);
                        auto partial = viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::Compile_time_error::Empty_error
                            , erased_location.at(0)
                        }.note("erased here");
                        for (auto each = erased_location.begin() + 1; each != erased_location.end(); ++each) {
                            partial.add(*each);
                        }
                        error.append(partial);
                    }
                    throw error;
                }

                auto const source_type_signature = std::vector<values::Value_type>{
                    values::Value_type::Vector
                    , values::Value_type::Value
                };
                if (not function_state.assume_type(source_index, source.register_set, source_type_signature)) {
                    auto error = viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::Compile_time_error::Type_mismatch
                            , source.tokens().at(1)
                            , "expected `" + to_string(source_type_signature) + "'..."
                        }.add(source.tokens().at(2)));

                    auto const& definition_location = function_state.defined_at(
                        source_index
                        , source.register_set
                    );
                    error.append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::Compile_time_error::Empty_error
                        , definition_location.at(0)
                        , ("...got `"
                           + to_string(function_state.type_of(source_index, source.register_set).to_simple())
                           + "'")
                    }.note("defined here"));
                    throw error;
                }

                auto const index_index = function_state.resolve_index(index);
                auto const index_type_signature = std::vector<values::Value_type>{
                    values::Value_type::Integer
                };
                if (not function_state.type_matches(index_index, index.register_set, index_type_signature)) {
                    auto error = viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::Compile_time_error::Type_mismatch
                            , index.tokens().at(1)
                            , "expected `" + to_string(index_type_signature) + "'..."
                        }.add(index.tokens().at(2)));

                    auto const& definition_location = function_state.defined_at(
                        index_index
                        , index.register_set
                    );
                    error.append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::Compile_time_error::Empty_error
                        , definition_location.at(0)
                        , ("...got `"
                           + to_string(function_state.type_of(index_index, index.register_set).to_simple())
                           + "'")
                    }.note("defined here"));
                    throw error;
                }

                auto const& dest = *static_cast<Register_address const*>(instruction.operands.at(0).get());
                auto defining_tokens = std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));
                std::copy(dest.tokens().begin(), dest.tokens().end(), std::back_inserter(defining_tokens));

                auto const dest_index = function_state.resolve_index(dest);
                function_state.define_register(
                    dest_index
                    , dest.register_set
                    , function_state.make_wrapper(std::make_unique<values::Pointer>(
                        static_cast<values::Vector const&>(
                            function_state.type_of(source_index, source.register_set).value()
                        ).of()
                    ))
                    , std::move(defining_tokens)
                );
            } else if (instruction.opcode == TEXT) {
                auto const& dest = *static_cast<Register_address const*>(instruction.operands.at(0).get());

                auto defining_tokens = std::vector<viua::tooling::libs::lexer::Token>{};
                defining_tokens.push_back(line->token(0));

                std::copy(dest.tokens().begin(), dest.tokens().end(), std::back_inserter(defining_tokens));

                using viua::tooling::libs::parser::Integer_literal;
                function_state.define_register(
                    function_state.resolve_index(dest)
                    , dest.register_set
                    , function_state.make_wrapper(std::make_unique<values::Text>())
                    , std::move(defining_tokens)
                );
            } else if (instruction.opcode == IINC or instruction.opcode == IDEC) {
                auto const& target = *static_cast<Register_address const*>(instruction.operands.at(0).get());

                auto const target_index = function_state.resolve_index(target);
                if (not function_state.defined(target_index, target.register_set)) {
                    auto error = viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::Compile_time_error::Access_to_empty_register
                            , target.tokens().at(1)
                        }.add(target.tokens().at(2)));
                    if (function_state.erased(target_index, target.register_set)) {
                        auto const& erased_location =
                            function_state.erased_at(target_index, target.register_set);
                        auto partial = viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::Compile_time_error::Empty_error
                            , erased_location.at(0)
                        }.note("erased here");
                        for (auto each = erased_location.begin() + 1; each != erased_location.end(); ++each) {
                            partial.add(*each);
                        }
                        error.append(partial);
                    }
                    throw error;
                }

                auto const target_access = target.access;
                auto const target_type_signature =
                    (target_access == viua::internals::Access_specifier::POINTER_DEREFERENCE)
                    ? std::vector<values::Value_type>{ values::Value_type::Pointer, values::Value_type::Integer }
                    : std::vector<values::Value_type>{ values::Value_type::Integer }
                ;
                if (not function_state.assume_type(target_index, target.register_set, target_type_signature)) {
                    auto error = viua::tooling::errors::compile_time::Error_wrapper{}
                        .append(viua::tooling::errors::compile_time::Error{
                            viua::tooling::errors::compile_time::Compile_time_error::Type_mismatch
                            , target.tokens().at(0)
                            , "expected `" + to_string(target_type_signature) + "'..."
                        }.add(target.tokens().at(0)));

                    auto const& definition_location = function_state.defined_at(
                        target_index
                        , target.register_set
                    );
                    error.append(viua::tooling::errors::compile_time::Error{
                        viua::tooling::errors::compile_time::Compile_time_error::Empty_error
                        , definition_location.at(0)
                        , ("...got `"
                           + to_string(function_state.type_of(target_index, target.register_set).to_simple())
                           + "'")
                    }.note("defined here"));
                    throw error;
                }
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