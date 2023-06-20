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
    auto operator=(Module&&) -> Module&      = delete;

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

template<typename> inline constexpr bool always_false_v = false;

struct Register {
    using void_type   = std::monostate;
    using int_type    = int64_t;
    using uint_type   = uint64_t;
    using float_type  = float;
    using double_type = double;
    struct pointer_type {
        uint64_t ptr;
    };
    struct atom_type {
        using key_type = uint64_t;
        key_type key;
    };
    using pid_type       = in6_addr;
    using undefined_type = std::array<uint8_t, sizeof(pid_type)>;

    enum class Types : uint8_t {
        VOID,
        INT,
        UINT,
        FLOAT32,
        FLOAT64,
        POINTER,
        ATOM,
        PID,
        UNDEFINED,
    };

    using value_type = std::variant<void_type,
                                    int_type,
                                    uint_type,
                                    float_type,
                                    double_type,
                                    pointer_type,
                                    atom_type,
                                    pid_type,
                                    undefined_type>;
    value_type value;

    std::optional<uint8_t> loaded_size;

    auto as_memory() const -> undefined_type;
    template<typename T> auto convert_undefined_to() -> void
    {
        auto const raw = std::get<undefined_type>(value);
        if constexpr (std::is_same_v<T, int_type>) {
            auto v = int_type{};
            memcpy(&v, raw.data(), sizeof(T));
            v = static_cast<T>(le64toh(v));

            /*
             * Sign-extend signed integers shorter than a full register.
             * Otherwise loading a signed byte, half-word, or word will produce
             * an incorrect result as the high bits will be zero.
             */
            auto const off = (64 - (*loaded_size * 8));
            v              = ((v << off) >> off);

            value = v;
        } else if constexpr (std::is_same_v<T, uint_type>) {
            auto v = uint_type{};
            memcpy(&v, raw.data(), sizeof(T));
            value = static_cast<T>(le64toh(v));
        } else if constexpr (std::is_same_v<T, float_type>) {
            auto v = float_type{};
            memcpy(&v, raw.data(), sizeof(v));
            value = v;
        } else if constexpr (std::is_same_v<T, double_type>) {
            auto v = double_type{};
            memcpy(&v, raw.data(), sizeof(v));
            value = v;
        } else if constexpr (std::is_same_v<T, pointer_type>) {
            auto tmp = uint64_t{};
            memcpy(&tmp, raw.data(), sizeof(T));
            tmp = le64toh(tmp);

            auto v = pointer_type{};
            memcpy(&v.ptr, &tmp, sizeof(v.ptr));

            value = v;
        } else if constexpr (std::is_same_v<T, atom_type>) {
            auto tmp = uint64_t{};
            memcpy(&tmp, raw.data(), sizeof(T));
            tmp = le64toh(tmp);

            auto v = atom_type{};
            memcpy(&v.key, &tmp, sizeof(v.key));

            value = v;
        } else if constexpr (std::is_same_v<T, pid_type>) {
            auto v = pid_type{};
            memcpy(&v, raw.data(), sizeof(v));

            value = v;
        } else if constexpr (std::is_same_v<T, undefined_type>) {
            /* do nothing */
        } else {
            static_assert(always_false_v<T>,
                          "invalid type convert to from undefined");
        }

        loaded_size.reset();
    }

    template<typename T> auto holds() const -> bool
    {
        if constexpr (std::is_same_v<T, void>) {
            return is_void();
        } else {
            return std::holds_alternative<T>(value);
        }
    }
    auto is_void() const -> bool
    {
        return std::holds_alternative<void_type>(value);
    }
    auto reset() -> void
    {
        value = void_type{};
    }

    template<typename T> auto get() const -> std::optional<T>
    {
        if (not std::holds_alternative<T>(value)) {
            return {};
        }
        return std::get<T>(value);
    }

    template<typename T> auto cast_to() const -> std::optional<T>
    {
        if (holds<void_type>()) {
            return {};
        }
        if (holds<pointer_type>()) {
            return {};
        }
        if (holds<pid_type>()) {
            return {};
        }

        if (auto v = get<int_type>(); v) {
            return static_cast<T>(*v);
        }
        if (auto v = get<uint_type>(); v) {
            return static_cast<T>(*v);
        }
        if (auto v = get<float_type>(); v) {
            return static_cast<T>(*v);
        }
        if (auto v = get<double_type>(); v) {
            return static_cast<T>(*v);
        }

        return {};
    }

    Register() = default;
    explicit Register(value_type v) : value{v}
    {}
    Register(Register const&) = delete;
    Register(Register&&)      = default;
    auto operator=(Register const& v) -> Register&
    {
        value = v.value;
        return *this;
    }
    auto operator=(Register&& v) -> Register&
    {
        value = std::move(v.value);
        return *this;
    }
    template<typename T> auto operator=(T const& v) -> Register&
    {
        value = v;
        return *this;
    }
    template<typename T> auto operator=(T&& v) -> Register&
    {
        if constexpr (std::is_same_v<T, bool>) {
            value = static_cast<uint_type>(v);
        } else {
            value = std::move(v);
        }
        return *this;
    }

    inline auto type_name() const -> std::string_view
    {
        if (holds<void_type>()) {
            return "void";
        } else if (holds<int_type>()) {
            return "int";
        } else if (holds<uint_type>()) {
            return "uint";
        } else if (holds<float_type>()) {
            return "float";
        } else if (holds<double_type>()) {
            return "double";
        } else if (holds<pointer_type>()) {
            return "ptr";
        } else if (holds<atom_type>()) {
            return "atom";
        } else if (holds<pid_type>()) {
            return "pid";
        } else {
            return "void";
        }
    }
};

/*
 * Why not 0, since null pointers are not possible?
 * Because we identify pointers by a tuple of {addr, parent.addr}, and origin
 * pointers (ie, those allocated by AA or AD instruction) have their parent set
 * to 0.
 *
 * If creating pointers at address zero was legal, it would not be possible to
 * distinguish origin pointers from subobjects of an area allocated at address
 * zero.
 */
inline constexpr auto MEM_FIRST_STACK_BREAK = size_t{0xbfff'ffff'ffff'fff0};

struct Frame {
    using addr_type = viua::arch::instruction_type const*;

    std::vector<Register> parameters;
    std::vector<Register> registers;

    addr_type const entry_address;
    addr_type const return_address;

    viua::arch::Register_access result_to;

    struct {
        uint64_t fp{MEM_FIRST_STACK_BREAK};
        uint64_t sbrk{MEM_FIRST_STACK_BREAK};
    } saved;

    inline Frame(size_t const sz, addr_type const e, addr_type const r)
            : registers(sz), entry_address{e}, return_address{r}
    {}
};

namespace io {
using buffer_view = std::basic_string_view<uint8_t>;
}

struct IO_request {
    using id_type = uint64_t;
    id_type const id{};

    using opcode_type = decltype(io_uring_sqe::opcode);
    opcode_type const opcode{};

    using buffer_type = std::string;
    std::variant<io::buffer_view, buffer_type> buffer;

    uint8_t* const req_ptr{nullptr};

    enum class Status {
        In_flight,
        Executing,
        Success,
        Error,
        Cancel,
    };
    Status status{Status::In_flight};

    inline IO_request(uint8_t* const rp,
                      id_type const i,
                      opcode_type const o,
                      buffer_type b)
            : id{i}, opcode{o}, buffer{b}, req_ptr{rp}
    {}
    inline IO_request(uint8_t* const rp,
                      id_type const i,
                      opcode_type const o,
                      io::buffer_view b)
            : id{i}, opcode{o}, buffer{b}, req_ptr{rp}
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
    auto schedule(uint8_t* const, int const, opcode_type const, io::buffer_view)
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

struct Process;

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
    inline auto push_ready(std::experimental::observer_ptr<Process> proc)
        -> void
    {
        run_queue.push(std::move(proc));
    }
    auto find(pid_type const) -> std::experimental::observer_ptr<Process>;

    auto spawn(std::string, uint64_t const) -> pid_type;
};

struct Stack {
    using addr_type = viua::arch::instruction_type const*;

    Process* proc;

    addr_type ip{nullptr};

    std::vector<Frame> frames;
    std::vector<Register> args;

    explicit inline Stack(Process& p) : proc{&p}
    {}

    Stack(Stack const&)                      = delete;
    Stack(Stack&&)                           = default;
    auto operator=(Stack const&) -> Stack&   = delete;
    inline auto operator=(Stack&&) -> Stack& = default;

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

/*
 * Why 16?
 * Because it means that line addresses in hexadecimal are 0, 10, 20, etc.
 */
inline constexpr auto MEM_LINE_SIZE = size_t{16};
inline constexpr auto MEM_PAGE_SIZE = MEM_LINE_SIZE * 16;

struct Page {
    using unit_type    = uint8_t;
    using storage_type = std::array<unit_type, MEM_PAGE_SIZE>;

    alignas(uint64_t) storage_type storage;

    inline constexpr auto data() -> uint8_t*
    {
        return storage.data();
    }
    inline constexpr auto data() const -> uint8_t const*
    {
        return storage.data();
    }

    inline constexpr auto at(size_t const i) -> uint8_t&
    {
        return storage.at(i);
    }
    inline constexpr auto at(size_t const i) const -> uint8_t
    {
        return storage.at(i);
    }

    inline constexpr auto size() const -> size_t
    {
        return storage.size();
    }
};

struct Pointer {
    /*
     * Foreign pointer is a pointer to memory that was not allocated on the
     * process' stack and heap managed by the VM.
     * These are pointers to host OS structures, objects received from FFI, etc.
     */
    bool foreign{false};

    /*
     * For foreign pointers: host OS pointer.
     * For VM pointers: offset from stack break 0.
     */
    uintptr_t ptr{0};

    /*
     * The page() and offset() functions only make sense for VM pointers, where
     * the pointer is really an offset into the vector of pages allocated for a
     * process' stack.
     */
    inline auto page() const -> size_t
    {
        return (ptr / MEM_PAGE_SIZE);
    }
    inline auto offset() const -> size_t
    {
        return (ptr % MEM_PAGE_SIZE);
    }

    /*
     * For foreign pointers: 0.
     * For VM pointers: size of the area pointed to.
     */
    size_t size{0};

    /*
     * This member is only non-zero for VM pointers which were created by
     * pointer arithmetic (ie, not allocated by AA or AD instruction). It serves
     * to identify overlapping memory areas eg,
     *
     *      +-----------------+
     *      | SUBOBJECT  |    |
     *      +------------+    |
     *      |                 |
     *      |     OBJECT      |
     *      |                 |
     *      +-----------------+
     *
     * The OBJECT must have been allocated using either AA or AD instruction.
     * Its parent field will be 0.
     *
     * The SUBOBJECT must have been created using either ADDIU or ADD
     * instruction to which the lhs operand was a pointer to the OBJECT. Its
     * parent field will be set to address of OBJECT.
     */
    uintptr_t parent{0};

    using id_type = decltype(ptr);
    inline auto id() const -> id_type
    {
        return ptr;
    }
};

struct Process {
    using pid_type = viua::runtime::PID;
    pid_type const pid;

    Core* core{};

    Module const& module;
    Module::strtab_type const* strtab;

    /*
     * Atoms are stored in .rodata and only instantiated when first referenced
     * by an instruction executed by the process. Since they can be bigger than
     * 128 bits and the VM is not able to hold them directly in a register, we
     * need to use a trick.
     *
     * What we really do when executing an ATOM instruction is we create a
     * useful object representing the atom, stash it aside, and store the key
     * with which it can be referenced in the register.
     *
     * We can easily check if the atom was already instantiated so there is no
     * need to produce multiple copies of it. When comparing atoms we can also
     * do it by key and we do not need to compare their text contents.
     */
    using atom_key_type  = Register::atom_type::key_type;
    using atoms_map_type = std::map<atom_key_type, std::string>;
    atoms_map_type atoms;

    using globals_map_type = std::map<atom_key_type, Register>;
    globals_map_type globals;

    using stack_type = Stack;
    stack_type stack;

    std::vector<Page> memory;
    std::map<Pointer::id_type, Pointer> pointers;
    uint64_t frame_pointer{MEM_FIRST_STACK_BREAK + 1};
    uint64_t stack_break{MEM_FIRST_STACK_BREAK + 1};

    auto memory_at(size_t const ptr) -> uint8_t*;
    auto record_pointer(Pointer ptr) -> void;
    auto forget_pointer(Pointer ptr) -> void;
    auto get_pointer(uint64_t addr) const -> std::optional<Pointer>;
    auto prune_pointers() -> void;

    explicit inline Process(pid_type const p, Core* c, Module const& m)
            : pid{p}, core{c}, module{m}, strtab{&m.strings_table}, stack{*this}
    {
        memory.emplace_back();
    }

    inline auto push_frame(size_t const locals,
                           stack_type::addr_type const entry_ip,
                           stack_type::addr_type const return_ip) -> void
    {
        stack.push(locals, entry_ip, return_ip);
        stack.ip = entry_ip;
    }
};

struct abort_execution {
    using stack_type = viua::vm::Stack;
    stack_type const& stack;
    std::string message;

    inline abort_execution(stack_type const& s, std::string m = "")
            : stack{s}, message{std::move(m)}
    {}

    inline auto what() const -> std::string_view
    {
        return message;
    }
};
}  // namespace viua::vm

#endif
