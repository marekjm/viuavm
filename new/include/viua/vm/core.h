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

#include <liburing.h>

#include <atomic>
#include <unordered_map>
#include <exception>
#include <memory>
#include <optional>
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
    using value_type = viua::vm::types::Cell;
    using boxed_type = value_type::boxed_type;
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
        value.content = v;
        return *this;
    }
    inline auto operator=(uint64_t const v) -> Value&
    {
        value.content = v;
        return *this;
    }
    inline auto operator=(float const v) -> Value&
    {
        value.content = v;
        return *this;
    }
    inline auto operator=(double const v) -> Value&
    {
        value.content = v;
        return *this;
    }
    template<typename T> inline auto operator=(T&& v) -> Value&
    {
        value.content = std::move(v);
        return *this;
    }

    inline auto is_boxed() const -> bool
    {
        return std::holds_alternative<boxed_type>(value.content);
    }
    inline auto is_void() const -> bool
    {
        return std::holds_alternative<std::monostate>(value.content);
    }
    inline auto make_void() -> void
    {
        value.content = std::monostate{};
    }

    inline auto boxed_value() const -> boxed_type::element_type const&
    {
        return *std::get<boxed_type>(value.content);
    }
    inline auto boxed_value() -> boxed_type::element_type&
    {
        return *std::get<boxed_type>(value.content);
    }
    template<typename T>
    inline auto boxed_of() -> std::optional<std::reference_wrapper<T>>
    {
        if (holds<T>()) {
            return static_cast<T&>(*std::get<boxed_type>(value.content));
        } else {
            return {};
        }
    }
    template<typename T>
    inline auto boxed_of() const
        -> std::optional<std::reference_wrapper<T const>>
    {
        if (holds<T>()) {
            return static_cast<T const&>(*std::get<boxed_type>(value.content));
        } else {
            return {};
        }
    }

    inline auto value_cell() -> viua::vm::types::Cell&
    {
        if (is_void()) {
            throw std::bad_cast{};
        }
        return value;
    }

    template<typename T> auto holds() const -> bool
    {
        if (std::is_same_v<
                T,
                void> and std::holds_alternative<std::monostate>(value.content)) {
            return true;
        }
        if (std::is_same_v<
                T,
                int64_t> and std::holds_alternative<int64_t>(value.content)) {
            return true;
        }
        if (std::is_same_v<
                T,
                uint64_t> and std::holds_alternative<uint64_t>(value.content)) {
            return true;
        }
        if (std::is_same_v<
                T,
                float> and std::holds_alternative<float>(value.content)) {
            return true;
        }
        if (std::is_same_v<
                T,
                double> and std::holds_alternative<double>(value.content)) {
            return true;
        }
        if (std::is_same_v<
                T,
                boxed_type> and std::holds_alternative<boxed_type>(value.content)) {
            return true;
        }
        if constexpr (std::is_convertible_v<T*, viua::vm::types::Value*>) {
            if (std::holds_alternative<boxed_type>(value.content)
                and dynamic_cast<T*>(
                    std::get<boxed_type>(value.content).get())) {
                return true;
            }
        }
        return false;
    }
    template<typename T> auto cast_to() const -> T
    {
        if (std::holds_alternative<T>(value.content)) {
            return std::get<T>(value.content);
        }

        if (std::holds_alternative<int64_t>(value.content)) {
            return static_cast<T>(std::get<int64_t>(value.content));
        }
        if (std::holds_alternative<uint64_t>(value.content)) {
            return static_cast<T>(std::get<uint64_t>(value.content));
        }
        if (std::holds_alternative<float>(value.content)) {
            return static_cast<T>(std::get<float>(value.content));
        }
        if (std::holds_alternative<double>(value.content)) {
            return static_cast<T>(std::get<double>(value.content));
        }

        using viua::vm::types::Float_double;
        using viua::vm::types::Float_single;
        using viua::vm::types::Signed_integer;
        using viua::vm::types::Unsigned_integer;
        if (holds<Signed_integer>()) {
            return static_cast<T>(
                static_cast<Signed_integer const&>(boxed_value()).value);
        }
        if (holds<Unsigned_integer>()) {
            return static_cast<T>(
                static_cast<Unsigned_integer const&>(boxed_value()).value);
        }
        if (holds<Float_single>()) {
            return static_cast<T>(
                static_cast<Float_single const&>(boxed_value()).value);
        }
        if (holds<Float_double>()) {
            return static_cast<T>(
                static_cast<Float_double const&>(boxed_value()).value);
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

struct IO_request {
    using id_type = uint64_t;
    id_type const id {};

    using opcode_type = decltype(io_uring_sqe::opcode);
    opcode_type const opcode {};

    using buffer_type = std::string;
    buffer_type buffer {};

    enum class Status {
        In_flight,
        Executing,
        Success,
        Error,
        Cancel,
    };
    Status status { Status::In_flight };

    inline IO_request(id_type const i, opcode_type const o, buffer_type b)
        : id{i}
        , opcode{o}
        , buffer{b}
    {}
};

struct IO_scheduler {
    inline static constexpr auto IO_URING_ENTRIES = size_t{4096};
    io_uring ring;

    using id_type = std::atomic<IO_request::id_type>;
    id_type next_id;

    using map_type = std::unordered_map<IO_request::id_type, std::unique_ptr<IO_request>>;
    map_type requests;

    inline IO_scheduler()
    {
        io_uring_queue_init(IO_URING_ENTRIES, &ring, 0);
    }
    inline ~IO_scheduler()
    {
        io_uring_queue_exit(&ring);
    }

    using opcode_type = decltype(io_uring_sqe::opcode);
    using buffer_type = IO_request::buffer_type;
    auto schedule(int const, opcode_type const, buffer_type) -> IO_request::id_type;
};

struct Stack {
    using addr_type = viua::arch::instruction_type const*;

    IO_scheduler io;

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
