/*
 *  Copyright (C) 2021 Marek Marecki
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

#ifndef VIUA_VM_CORE_H
#define VIUA_VM_CORE_H

#include <endian.h>
#include <string.h>

#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/vm/types.h>


namespace viua::vm {
struct Env {
    std::vector<uint8_t> strings_table;
    std::vector<uint8_t> functions_table;
    viua::arch::instruction_type const* ip_base;

    auto function_at(size_t const) const -> std::pair<std::string, size_t>;
};

struct Value {
    using boxed_type = std::unique_ptr<viua::vm::types::Value>;
    using value_type = viua::vm::types::Register_cell;
    value_type value;

    Value()             = default;
    Value(Value const&) = delete;
    Value(Value&&)      = delete;
    auto operator=(Value const&) -> Value& = delete;
    inline auto operator                   =(Value&& that) -> Value&
    {
        value = std::move(that.value);
        return *this;
    }
    ~Value() = default;

    inline auto operator=(bool const v) -> Value&
    {
        return (*this = static_cast<uint64_t>(v));
    }
    inline auto operator=(int64_t const v) -> Value&
    {
        value = v;
        return *this;
    }
    inline auto operator=(uint64_t const v) -> Value&
    {
        value = v;
        return *this;
    }
    inline auto operator=(float const v) -> Value&
    {
        value = v;
        return *this;
    }
    inline auto operator=(double const v) -> Value&
    {
        value = v;
        return *this;
    }

    inline auto is_boxed() const -> bool
    {
        return std::holds_alternative<boxed_type>(value);
    }
    inline auto is_void() const -> bool
    {
        return std::holds_alternative<std::monostate>(value);
    }
    inline auto make_void() -> void
    {
        value = std::monostate{};
    }

    inline auto boxed_value() const -> boxed_type::element_type const&
    {
        return *std::get<boxed_type>(value);
    }
    inline auto boxed_value() -> boxed_type::element_type&
    {
        return *std::get<boxed_type>(value);
    }
    inline auto value_cell() -> viua::vm::types::Value_cell
    {
        if (holds<int64_t>()) {
            auto t = std::get<int64_t>(value);
            make_void();
            return t;
        } else if (holds<uint64_t>()) {
            auto t = std::get<uint64_t>(value);
            make_void();
            return t;
        } else if (holds<float>()) {
            auto t = std::get<float>(value);
            make_void();
            return t;
        } else if (holds<double>()) {
            auto t = std::get<double>(value);
            make_void();
            return t;
        } else if (holds<boxed_type>()) {
            auto t = std::move(std::get<boxed_type>(value));
            make_void();
            return t;
        } else {
            throw std::bad_cast{};
        }
    }

    template<typename T> auto holds() const -> bool
    {
        if (std::is_same_v<
                T,
                void> and std::holds_alternative<std::monostate>(value)) {
            return true;
        }
        if (std::is_same_v<
                T,
                int64_t> and std::holds_alternative<int64_t>(value)) {
            return true;
        }
        if (std::is_same_v<
                T,
                uint64_t> and std::holds_alternative<uint64_t>(value)) {
            return true;
        }
        if (std::is_same_v<T, float> and std::holds_alternative<float>(value)) {
            return true;
        }
        if (std::is_same_v<T,
                           double> and std::holds_alternative<double>(value)) {
            return true;
        }
        if (std::is_same_v<
                T,
                boxed_type> and std::holds_alternative<boxed_type>(value)) {
            return true;
        }
        if (std::is_convertible_v<
                T*,
                viua::vm::types::Value*> and std::holds_alternative<boxed_type>(value)) {
            return true;
        }
        return false;
    }
    template<typename T> auto cast_to() const -> T
    {
        if (std::holds_alternative<T>(value)) {
            return std::get<T>(value);
        }

        if (std::holds_alternative<int64_t>(value)) {
            return static_cast<T>(std::get<int64_t>(value));
        }
        if (std::holds_alternative<uint64_t>(value)) {
            return static_cast<T>(std::get<uint64_t>(value));
        }
        if (std::holds_alternative<float>(value)) {
            return static_cast<T>(std::get<float>(value));
        }
        if (std::holds_alternative<double>(value)) {
            return static_cast<T>(std::get<double>(value));
        }

        throw std::bad_cast{};
    }

    template<typename Tr> auto has_trait() const -> bool
    {
        return (is_boxed() and boxed_value().has_trait<Tr>());
    }
};

struct Frame {
    using addr_type = viua::arch::instruction_type const*;

    std::vector<Value> parameters;
    std::vector<Value> registers;

    addr_type const entry_address;
    addr_type const return_address;

    viua::arch::Register_access result_to;

    inline Frame(size_t const sz, addr_type const e, addr_type const r)
            : registers(sz), entry_address{e}, return_address{r}
    {}
};

struct Stack {
    using addr_type = viua::arch::instruction_type const*;

    Env const& environment;
    std::vector<Frame> frames;
    std::vector<Value> args;

    explicit inline Stack(Env const& env) : environment{env}
    {}
    Stack(Stack const&) = delete;
    Stack(Stack&&)      = default;
    auto operator=(Stack const&) -> Stack& = delete;
    auto operator=(Stack&&) -> Stack& = default;

    inline auto push(size_t const sz, addr_type const e, addr_type const r)
        -> void
    {
        frames.emplace_back(sz, e, r);
    }

    inline auto back() -> decltype(frames)::reference
    {
        return frames.back();
    }
    inline auto back() const -> decltype(frames)::const_reference
    {
        return frames.back();
    }
};

struct abort_execution {
    using ip_type = viua::arch::instruction_type const*;
    ip_type const ip;
    std::string message;

    inline abort_execution(ip_type const i, std::string m = "")
            : ip{i}, message{std::move(m)}
    {}

    inline auto what() const -> std::string_view
    {
        return message;
    }
};
}  // namespace viua::vm

#endif
