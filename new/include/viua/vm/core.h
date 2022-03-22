/*
 *  Copyright (C) 2021-2022 Marek Marecki
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
#include <liburing.h>
#include <string.h>

#include <atomic>
#include <chrono>
#include <exception>
#include <experimental/memory>
#include <filesystem>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/runtime/pid.h>
#include <viua/vm/elf.h>
#include <viua/vm/types.h>


namespace viua::vm {
struct Module {
    using strtab_type = std::vector<uint8_t>;
    using fntab_type  = std::vector<uint8_t>;
    using text_type   = std::vector<viua::arch::instruction_type>;

    std::filesystem::path const elf_path;
    viua::vm::elf::Loaded_elf const elf;

    strtab_type const& strings_table;
    fntab_type const& functions_table;

    text_type const text;
    text_type::value_type const* ip_base;

    inline Module(std::filesystem::path const ep, viua::vm::elf::Loaded_elf le)
            : elf_path{std::move(ep)}
            , elf{std::move(le)}
            , strings_table{elf.find_fragment(".rodata")->get().data}
            , functions_table{elf.find_fragment(".viua.fns")->get().data}
            , text{elf.make_text_from(elf.find_fragment(".text")->get().data)}
            , ip_base{text.data()}
    {}
    inline Module(Module const&) = delete;
    inline Module(Module&& m) : Module{std::move(m.elf_path), std::move(m.elf)}
    {}
    auto operator=(Module const&) -> Module& = delete;
    auto operator=(Module&&) -> Module& = delete;

    inline auto function_at(size_t const off) const
        -> std::pair<std::string, size_t>
    {
        return elf.fn_at(functions_table, off);
    }

    inline auto ip_in_valid_range(text_type::value_type const* ip) const -> bool
    {
        return (ip > ip_base) and (ip < (ip_base + text.size()));
    }
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
    id_type const id{};

    using opcode_type = decltype(io_uring_sqe::opcode);
    opcode_type const opcode{};

    using buffer_type = std::string;
    buffer_type buffer{};

    enum class Status {
        In_flight,
        Executing,
        Success,
        Error,
        Cancel,
    };
    Status status{Status::In_flight};

    inline IO_request(id_type const i, opcode_type const o, buffer_type b)
            : id{i}, opcode{o}, buffer{b}
    {}
};

struct IO_scheduler {
    inline static constexpr auto IO_URING_ENTRIES = size_t{4096};
    io_uring ring;

    using id_type = std::atomic<IO_request::id_type>;
    id_type next_id;

    using map_type =
        std::unordered_map<IO_request::id_type, std::unique_ptr<IO_request>>;
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
    auto schedule(int const, opcode_type const, buffer_type)
        -> IO_request::id_type;
};

struct Performance_counters {
    using counter_type = uint64_t;
    counter_type total_ops_executed{0};
    counter_type total_us_elapsed{0};

    using time_point_type = std::chrono::time_point<std::chrono::steady_clock>;
    time_point_type bang{};
    time_point_type death{};

    inline auto start() -> void
    {
        bang = std::chrono::steady_clock::now();
    }
    inline auto stop() -> void
    {
        death = std::chrono::steady_clock::now();
    }
    inline auto duration() const -> auto
    {
        return (death - bang);
    }
};

class Process;

struct Core {
    std::map<std::string, Module> modules;

    IO_scheduler io;

    Performance_counters perf_counters;

    using pid_type = viua::runtime::PID;
    viua::runtime::Pid_emitter pids;

    std::map<pid_type, std::unique_ptr<Process>> flock;
    std::queue<std::experimental::observer_ptr<Process>> run_queue;
    std::map<pid_type, std::experimental::observer_ptr<Process>> suspended;

    inline auto pop_ready() -> auto
    {
        auto proc = std::move(run_queue.front());
        run_queue.pop();
        return proc;
    }
    inline auto push_ready(std::experimental::observer_ptr<Process> proc) -> void
    {
        run_queue.push(std::move(proc));
    }

    auto spawn(std::string, uint64_t const) -> pid_type;
};

struct Stack {
    using addr_type = viua::arch::instruction_type const*;

    Process& proc;

    addr_type ip{nullptr};

    std::vector<Frame> frames;
    std::vector<Value> args;

    explicit inline Stack(Process& p) : proc{p}
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

struct Process {
    using pid_type = viua::runtime::PID;
    pid_type const pid;

    Core* core{};
    Module const& module;

    using stack_type = Stack;
    stack_type stack;

    explicit inline Process(pid_type const p, Core* c, Module const& m)
            : pid{p}, core{c}, module{m}, stack{*this}
    {}

    inline auto push_frame(size_t const locals,
                           stack_type::addr_type const entry_ip,
                           stack_type::addr_type const return_ip) -> void
    {
        stack.push(locals, entry_ip, return_ip);
        stack.ip = entry_ip;
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
