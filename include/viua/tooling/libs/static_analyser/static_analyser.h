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

#ifndef VIUA_TOOLING_LIBS_STATIC_ANALYSER_H
#define VIUA_TOOLING_LIBS_STATIC_ANALYSER_H

#include <set>
#include <viua/tooling/libs/parser/parser.h>

namespace viua {
    namespace tooling {
        namespace libs {
            namespace static_analyser {
struct Analyser_state {
    std::set<std::string> functions_called;
};

namespace values {
enum class Value_type {
    Value,
    Integer,
    Float,
    Vector,
    String,
    Text,
    Pointer,
    Boolean,
    Bits,
    Closure,
    Function,
    Atom,
    Struct,
    Pid,
};

class Value {
    Value_type const type_of_value;

  public:
    auto type() const -> Value_type;

    static auto clone(Value const&, std::vector<std::unique_ptr<Value>>&)
        -> std::unique_ptr<Value>;

    Value(Value_type const);
};

class Value_wrapper {
  public:
    using map_type   = std::vector<std::unique_ptr<values::Value>>;
    using index_type = map_type::size_type;

  private:
    index_type i;
    map_type* values;

  public:
    auto value() const -> values::Value&;
    auto value(std::unique_ptr<values::Value>) -> void;
    virtual auto to_simple() const -> std::vector<values::Value_type>;

    auto index() const -> index_type;

    auto rebind(map_type&) const -> Value_wrapper;

    Value_wrapper(index_type const, map_type&);
    Value_wrapper(Value_wrapper const&);
    Value_wrapper(Value_wrapper&&) = default;
    auto operator                  =(Value_wrapper const&) -> Value_wrapper&;
    auto operator                  =(Value_wrapper &&) -> Value_wrapper&;
    virtual ~Value_wrapper();
};

class Integer : public Value {
    std::optional<int> n;

  public:
    auto known() const -> bool;
    auto of() const -> int;
    auto of(int const) -> void;

    Integer();
    Integer(int const);
};

class Float : public Value {
  public:
    Float();
};

class Vector : public Value {
    Value_wrapper contained_type;

  public:
    auto of() const -> Value_wrapper const&;
    auto of(Value_wrapper) -> void;

    Vector(Value_wrapper);
};

class String : public Value {
  public:
    String();
};

class Text : public Value {
  public:
    Text();
};

class Pointer : public Value {
    Value_wrapper contained_type;

  public:
    auto of() const -> Value_wrapper const&;
    auto of(Value_wrapper) -> void;

    Pointer(Value_wrapper);
};

class Boolean : public Value {
  public:
    Boolean();
};

class Bits : public Value {
  public:
    Bits();
};

class Closure : public Value {
    std::string name;

  public:
    auto of() const -> std::string;

    Closure(std::string name = "");
};

class Function : public Value {
    std::string name;

  public:
    auto of() const -> std::string;

    Function(std::string name = "");
};

class Atom : public Value {
    std::optional<std::string> value;

  public:
    Atom();
    Atom(std::string);

    auto known() const -> bool;
    auto of() const -> std::string;
};

class Struct : public Value {
    std::map<std::string, Value_wrapper> known_fields;

  public:
    Struct();

    auto fields() const -> decltype(known_fields) const&;
    auto field(std::string const&) const -> std::optional<Value_wrapper>;
    auto field(std::string, Value_wrapper) -> void;
};

class Pid : public Value {
  public:
    Pid();
};

class IO_port : public Value {
  public:
    IO_port();
};

class IO_request : public Value {
  public:
    IO_request();
};
}  // namespace values

class Function_state {
    viua::internals::types::register_index const local_registers_allocated = 0;
    std::vector<viua::tooling::libs::lexer::Token>
        local_registers_allocated_where;

    std::map<viua::internals::types::register_index,
             viua::tooling::libs::parser::Name_directive>
        register_renames;
    std::map<std::string, viua::internals::types::register_index>
        register_name_to_index;
    std::map<viua::internals::types::register_index, std::string>
        register_index_to_name;

    viua::internals::types::register_index iota_value = 1;

    std::vector<std::unique_ptr<values::Value>> assigned_values;

    using Register_address_type =
        std::pair<viua::internals::types::register_index,
                  viua::bytecode::codec::Register_set>;
    std::map<Register_address_type, values::Value_wrapper> defined_registers;
    std::map<Register_address_type,
             std::vector<viua::tooling::libs::lexer::Token>>
        defined_where;
    std::map<Register_address_type,
             std::vector<std::vector<viua::tooling::libs::lexer::Token>>>
        mutated_where;
    std::map<Register_address_type, values::Value_wrapper> erased_registers;
    std::map<Register_address_type,
             std::vector<viua::tooling::libs::lexer::Token>>
        erased_where;

  public:
    auto make_wrapper(std::unique_ptr<values::Value>) -> values::Value_wrapper;

    auto rename_register(viua::internals::types::register_index const,
                         std::string,
                         viua::tooling::libs::parser::Name_directive) -> void;

    auto define_register(viua::internals::types::register_index const,
                         viua::bytecode::codec::Register_set const,
                         values::Value_wrapper,
                         std::vector<viua::tooling::libs::lexer::Token>)
        -> void;
    auto defined(viua::internals::types::register_index const,
                 viua::bytecode::codec::Register_set const) const -> bool;
    auto defined_at(viua::internals::types::register_index const,
                    viua::bytecode::codec::Register_set const) const
        -> std::vector<viua::tooling::libs::lexer::Token> const&;
    auto type_of(viua::internals::types::register_index const,
                 viua::bytecode::codec::Register_set const) const
        -> values::Value_wrapper;
    auto mutate_register(viua::internals::types::register_index const,
                         viua::bytecode::codec::Register_set const,
                         std::vector<viua::tooling::libs::lexer::Token>)
        -> void;
    auto mutated(viua::internals::types::register_index const,
                 viua::bytecode::codec::Register_set const) const -> bool;
    auto mutated_at(viua::internals::types::register_index const,
                    viua::bytecode::codec::Register_set const) const
        -> decltype(mutated_where)::mapped_type const&;
    auto erase_register(viua::internals::types::register_index const,
                        viua::bytecode::codec::Register_set const,
                        std::vector<viua::tooling::libs::lexer::Token>) -> void;
    auto erased(viua::internals::types::register_index const,
                viua::bytecode::codec::Register_set const) const -> bool;
    auto erased_at(viua::internals::types::register_index const,
                   viua::bytecode::codec::Register_set const) const
        -> std::vector<viua::tooling::libs::lexer::Token> const&;

    auto iota(viua::tooling::libs::lexer::Token)
        -> viua::internals::types::register_index;

    auto resolve_index(viua::tooling::libs::parser::Register_address const&)
        -> viua::internals::types::register_index;

    auto type_matches(viua::internals::types::register_index const,
                      viua::bytecode::codec::Register_set const,
                      std::vector<values::Value_type> const) const -> bool;

  private:
    auto fill_type(values::Value_wrapper,
                   std::vector<values::Value_type> const&,
                   std::vector<values::Value_type>::size_type const) -> void;

  public:
    auto assume_type(viua::internals::types::register_index const,
                     viua::bytecode::codec::Register_set const,
                     std::vector<values::Value_type> const) -> bool;

    auto dump(std::ostream&) const -> void;
    auto clone() const -> Function_state;

    auto local_capacity() const -> viua::internals::types::register_index;

    Function_state(viua::internals::types::register_index const,
                   std::vector<viua::tooling::libs::lexer::Token>);
    Function_state(Function_state const&) = delete;
    Function_state(Function_state&&);
    auto operator=(Function_state const&) = delete;
    auto operator=(Function_state&&) = delete;
};

auto analyse(viua::tooling::libs::parser::Cooked_fragments const&) -> void;

}}}}  // namespace viua::tooling::libs::static_analyser

#endif
