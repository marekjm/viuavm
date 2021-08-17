#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/arch/ins.h>
#include <viua/arch/ops.h>
#include <viua/support/string.h>


namespace viua::vm::types {
namespace traits {
struct To_string {
    virtual auto to_string() const -> std::string = 0;
    virtual ~To_string();

    static auto quote_and_escape(std::string_view const sv) -> std::string
    {
        return viua::support::string::quoted(sv);
    }
};
}  // namespace traits

struct Value {
    virtual auto type_name() const -> std::string = 0;
    virtual ~Value();

    template<typename Trait>
    auto as_trait(std::function<void(Trait const&)> fn) const -> void
    {
        if (not has_trait<Trait>()) {
            return;
        }

        fn(*dynamic_cast<Trait const*>(this));
    }

    template<typename Trait> auto has_trait() const -> bool
    {
        return (dynamic_cast<Trait const*>(this) != nullptr);
    }
};

struct String
        : Value
        , traits::To_string {
    std::string content;

    auto type_name() const -> std::string override;
    auto to_string() const -> std::string override;
};
}  // namespace viua::vm::types

namespace viua::vm::types {
Value::~Value()
{}

auto String::type_name() const -> std::string
{
    return "string";
}
auto String::to_string() const -> std::string
{
    return viua::vm::types::traits::To_string::quote_and_escape(content);
}
}  // namespace viua::vm::types
namespace viua::vm::types::traits {
To_string::~To_string()
{}
}  // namespace viua::vm::types::traits

struct Value {
    enum class Unboxed_type : uint8_t {
        Void = 0,
        Byte,
        Integer_signed,
        Integer_unsigned,
        Float_single,
        Float_double,
    };
    Unboxed_type type_of_unboxed;

    using boxed_type = std::unique_ptr<viua::vm::types::Value>;
    std::variant<uint64_t, boxed_type> value;

    auto is_boxed() const -> bool
    {
        return std::holds_alternative<boxed_type>(value);
    }
    auto is_void() const -> bool
    {
        return ((not is_boxed())
                and type_of_unboxed == Value::Unboxed_type::Void);
    }

    auto boxed_value() const -> boxed_type const&
    {
        return std::get<boxed_type>(value);
    }
};

namespace machine::core::ins {
auto execute(std::vector<Value>& registers,
             std::vector<uint8_t> const&,
             viua::arch::ins::ADD const op) -> void
{
    auto& out = registers.at(op.instruction.out.index);
    auto& lhs = registers.at(op.instruction.lhs.index);
    auto& rhs = registers.at(op.instruction.rhs.index);

    out.type_of_unboxed = lhs.type_of_unboxed;
    out.value = (std::get<uint64_t>(lhs.value) + std::get<uint64_t>(rhs.value));

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";
}
auto execute(std::vector<Value>& registers,
             std::vector<uint8_t> const&,
             viua::arch::ins::SUB const op) -> void
{
    auto& out = registers.at(op.instruction.out.index);
    auto& lhs = registers.at(op.instruction.lhs.index);
    auto& rhs = registers.at(op.instruction.rhs.index);

    out.type_of_unboxed = lhs.type_of_unboxed;
    out.value = (std::get<uint64_t>(lhs.value) - std::get<uint64_t>(rhs.value));

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";
}
auto execute(std::vector<Value>& registers,
             std::vector<uint8_t> const&,
             viua::arch::ins::MUL const op) -> void
{
    auto& out = registers.at(op.instruction.out.index);
    auto& lhs = registers.at(op.instruction.lhs.index);
    auto& rhs = registers.at(op.instruction.rhs.index);

    out.type_of_unboxed = lhs.type_of_unboxed;
    out.value = (std::get<uint64_t>(lhs.value) * std::get<uint64_t>(rhs.value));

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";
}
auto execute(std::vector<Value>& registers,
             std::vector<uint8_t> const&,
             viua::arch::ins::DIV const op) -> void
{
    auto& out = registers.at(op.instruction.out.index);
    auto& lhs = registers.at(op.instruction.lhs.index);
    auto& rhs = registers.at(op.instruction.rhs.index);

    out.type_of_unboxed = lhs.type_of_unboxed;
    out.value = (std::get<uint64_t>(lhs.value) / std::get<uint64_t>(rhs.value));

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.lhs.index))
               + ", $"
               + std::to_string(static_cast<int>(op.instruction.rhs.index))
               + "\n";
}

auto execute(std::vector<Value>& registers,
             std::vector<uint8_t> const&,
             viua::arch::ins::DELETE const op) -> void
{
    auto& target = registers.at(op.instruction.out.index);

    target.type_of_unboxed = Value::Unboxed_type::Void;
    target.value           = uint64_t{0};

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + "\n";
}

auto execute(std::vector<Value>& registers,
             std::vector<uint8_t> const& strings,
             viua::arch::ins::STRING const op) -> void
{
    auto& target = registers.at(op.instruction.out.index);

    auto const data_offset = std::get<uint64_t>(target.value);
    auto const data_size   = [strings, data_offset]() -> uint64_t {
        auto const size_offset = (data_offset - sizeof(uint64_t));
        auto tmp               = uint64_t{};
        memcpy(&tmp, &strings[size_offset], sizeof(uint64_t));
        return le64toh(tmp);
    }();

    auto s     = std::make_unique<viua::vm::types::String>();
    s->content = std::string{
        reinterpret_cast<char const*>(&strings[0] + data_offset), data_size};

    target.type_of_unboxed = Value::Unboxed_type::Void;
    target.value           = std::move(s);

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + "\n";
}

auto execute(std::vector<Value>& registers,
             std::vector<uint8_t> const&,
             viua::arch::ins::LUI const op) -> void
{
    auto& value           = registers.at(op.instruction.out.index);
    value.type_of_unboxed = Value::Unboxed_type::Integer_signed;
    value.value           = (op.instruction.immediate << 28);

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", " + std::to_string(op.instruction.immediate) + "\n";
}
auto execute(std::vector<Value>& registers,
             std::vector<uint8_t> const&,
             viua::arch::ins::LUIU const op) -> void
{
    auto& value           = registers.at(op.instruction.out.index);
    value.type_of_unboxed = Value::Unboxed_type::Integer_unsigned;
    value.value           = (op.instruction.immediate << 28);

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", " + std::to_string(op.instruction.immediate) + "\n";
}

auto execute(std::vector<Value>& registers,
             std::vector<uint8_t> const&,
             viua::arch::ins::ADDI const op) -> void
{
    auto& out = registers.at(op.instruction.out.index);
    auto const base =
        (op.instruction.in.is_void()
             ? 0
             : std::get<uint64_t>(registers.at(op.instruction.in.index).value));

    out.type_of_unboxed = Value::Unboxed_type::Integer_signed;
    out.value           = (base + op.instruction.immediate);

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", void"  // FIXME it's not always void
               + ", " + std::to_string(op.instruction.immediate) + "\n";
}
auto execute(std::vector<Value>& registers,
             std::vector<uint8_t> const&,
             viua::arch::ins::ADDIU const op) -> void
{
    auto& out = registers.at(op.instruction.out.index);
    auto const base =
        (op.instruction.in.is_void()
             ? 0
             : std::get<uint64_t>(registers.at(op.instruction.in.index).value));

    out.type_of_unboxed = Value::Unboxed_type::Integer_unsigned;
    out.value           = (base + op.instruction.immediate);

    std::cerr
        << "    " + viua::arch::ops::to_string(op.instruction.opcode) + " $"
               + std::to_string(static_cast<int>(op.instruction.out.index))
               + ", void"  // FIXME it's not always void
               + ", " + std::to_string(op.instruction.immediate) + "\n";
}

auto execute(std::vector<Value>& registers,
             std::vector<uint8_t> const&,
             viua::arch::ins::EBREAK const) -> void
{
    for (auto i = size_t{0}; i < registers.size(); ++i) {
        auto const& each = registers.at(i);
        if (each.is_void()) {
            continue;
        }

        std::cerr << "[" << std::setw(3) << i << "] ";

        if (each.is_boxed()) {
            auto const& value = *each.boxed_value();
            std::cerr << "<boxed> " << value.type_name();
            value.as_trait<viua::vm::types::traits::To_string>(
                [](viua::vm::types::traits::To_string const& val) -> void {
                    std::cerr << " = " << val.to_string();
                });
            std::cerr << "\n";
            continue;
        }

        switch (each.type_of_unboxed) {
        case Value::Unboxed_type::Void:
            break;
        case Value::Unboxed_type::Byte:
            std::cerr << "by " << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<uint8_t>(std::get<uint64_t>(each.value))
                      << "\n";
            break;
        case Value::Unboxed_type::Integer_signed:
            std::cerr << "is " << std::hex << std::setw(16) << std::setfill('0')
                      << std::get<uint64_t>(each.value) << " " << std::dec
                      << static_cast<int64_t>(std::get<uint64_t>(each.value))
                      << "\n";
            break;
        case Value::Unboxed_type::Integer_unsigned:
            std::cerr << "iu " << std::hex << std::setw(16) << std::setfill('0')
                      << std::get<uint64_t>(each.value) << " " << std::dec
                      << std::get<uint64_t>(each.value) << "\n";
            break;
        case Value::Unboxed_type::Float_single:
            std::cerr << "fl " << std::hex << std::setw(8) << std::setfill('0')
                      << static_cast<float>(std::get<uint64_t>(each.value))
                      << " " << std::dec
                      << static_cast<float>(std::get<uint64_t>(each.value))
                      << "\n";
            break;
        case Value::Unboxed_type::Float_double:
            std::cerr << "db " << std::hex << std::setw(16) << std::setfill('0')
                      << static_cast<double>(std::get<uint64_t>(each.value))
                      << " " << std::dec
                      << static_cast<double>(std::get<uint64_t>(each.value))
                      << "\n";
            break;
        }
    }
}

auto execute(std::vector<Value>& registers,
             std::vector<uint8_t> const& strings,
             viua::arch::instruction_type const* const ip)
    -> viua::arch::instruction_type const*
{
    auto const raw = *ip;

    auto const opcode = static_cast<viua::arch::opcode_type>(
        raw & viua::arch::ops::OPCODE_MASK);
    auto const format = static_cast<viua::arch::ops::FORMAT>(
        opcode & viua::arch::ops::FORMAT_MASK);

    switch (format) {
    case viua::arch::ops::FORMAT::T:
    {
        auto instruction = viua::arch::ops::T::decode(raw);
        switch (static_cast<viua::arch::ops::OPCODE_T>(opcode)) {
        case viua::arch::ops::OPCODE_T::ADD:
            execute(registers, strings, viua::arch::ins::ADD{instruction});
            break;
        case viua::arch::ops::OPCODE_T::MUL:
            execute(registers, strings, viua::arch::ins::MUL{instruction});
            break;
        default:
            std::cerr << "unimplemented T instruction\n";
            return nullptr;
        }
        break;
    }
    case viua::arch::ops::FORMAT::S:
    {
        auto instruction = viua::arch::ops::S::decode(raw);
        switch (static_cast<viua::arch::ops::OPCODE_S>(opcode)) {
        case viua::arch::ops::OPCODE_S::DELETE:
            execute(registers, strings, viua::arch::ins::DELETE{instruction});
            break;
        case viua::arch::ops::OPCODE_S::STRING:
            execute(registers, strings, viua::arch::ins::STRING{instruction});
            break;
        }
        break;
    }
    case viua::arch::ops::FORMAT::E:
    {
        auto instruction = viua::arch::ops::E::decode(raw);
        switch (static_cast<viua::arch::ops::OPCODE_E>(opcode)) {
        case viua::arch::ops::OPCODE_E::LUI:
            execute(registers, strings, viua::arch::ins::LUI{instruction});
            break;
        case viua::arch::ops::OPCODE_E::LUIU:
            execute(registers, strings, viua::arch::ins::LUIU{instruction});
            break;
        }
        break;
    }
    case viua::arch::ops::FORMAT::R:
    {
        auto instruction = viua::arch::ops::R::decode(raw);
        switch (static_cast<viua::arch::ops::OPCODE_R>(opcode)) {
        case viua::arch::ops::OPCODE_R::ADDI:
            execute(registers, strings, viua::arch::ins::ADDI{instruction});
            break;
        case viua::arch::ops::OPCODE_R::ADDIU:
            execute(registers, strings, viua::arch::ins::ADDIU{instruction});
            break;
        }
        break;
    }
    case viua::arch::ops::FORMAT::N:
    {
        std::cerr << "    " + viua::arch::ops::to_string(opcode) + "\n";
        switch (static_cast<viua::arch::ops::OPCODE_N>(opcode)) {
        case viua::arch::ops::OPCODE_N::NOOP:
            break;
        case viua::arch::ops::OPCODE_N::HALT:
            return nullptr;
        case viua::arch::ops::OPCODE_N::EBREAK:
            execute(registers,
                    strings,
                    viua::arch::ins::EBREAK{viua::arch::ops::N::decode(raw)});
            break;
        }
        break;
    }
    case viua::arch::ops::FORMAT::D:
    case viua::arch::ops::FORMAT::F:
        std::cerr << "unimplemented instruction: "
                  << viua::arch::ops::to_string(opcode) << "\n";
        return nullptr;
    }

    return (ip + 1);
}
}  // namespace machine::core::ins

namespace {
auto run_instruction(std::vector<Value>& registers,
                     std::vector<uint8_t> const& strings,
                     viua::arch::instruction_type const* ip)
    -> viua::arch::instruction_type const*
{
    auto instruction = viua::arch::instruction_type{};
    do {
        instruction = *ip;
        ip          = machine::core::ins::execute(registers, strings, ip);
    } while ((ip != nullptr) and (instruction & viua::arch::ops::GREEDY));

    return ip;
}

auto run(std::vector<Value>& registers,
         std::vector<uint8_t> const& strings,
         viua::arch::instruction_type const* ip,
         std::tuple<std::string_view const,
                    viua::arch::instruction_type const*,
                    viua::arch::instruction_type const*> ip_range) -> void
{
    auto const [module, ip_begin, ip_end] = ip_range;

    constexpr auto PREEMPTION_THRESHOLD = size_t{2};

    while ((ip < ip_end) and (ip >= ip_begin)) {
        auto const ip_before = ip;

        std::cerr << "cycle at " << module << "+0x" << std::hex << std::setw(8)
                  << std::setfill('0')
                  << ((ip - ip_begin) * sizeof(viua::arch::instruction_type))
                  << std::dec << "\n";

        for (auto i = size_t{0}; i < PREEMPTION_THRESHOLD and ip != ip_end;
             ++i) {
            /*
             * This is needed to detect greedy bundles and adjust preemption
             * counter appropriately. If a greedy bundle contains more
             * instructions than the preemption threshold allows the process
             * will be suspended immediately.
             */
            auto const greedy    = (*ip & viua::arch::ops::GREEDY);
            auto const bundle_ip = ip;

            std::cerr << "  " << (greedy ? "bundle" : "single") << " "
                      << std::hex << std::setw(2) << std::setfill('0') << i
                      << std::dec << "\n";

            ip = run_instruction(registers, strings, ip);

            /*
             * Halting instruction returns nullptr because it does not know
             * where the end of bytecode lies. This is why we have to watch
             * out for null pointer here.
             */
            ip = (ip == nullptr ? ip_end : ip);

            /*
             * If the instruction was a greedy bundle instead of a single
             * one, the preemption counter has to be adjusted. It may be the
             * case that the bundle already hit the preemption threshold.
             */
            if (greedy and ip != ip_end) {
                i += (ip - bundle_ip) - 1;
            }
        }

        if (ip == ip_end) {
            std::cerr << "halted\n";
            break;
        } else {
            std::cerr << "preempted after " << (ip - ip_before) << " ops\n";
        }

        if constexpr (false) {
            /*
             * FIXME Limit the amount of instructions executed per second
             * for debugging purposes. Once everything works as it should,
             * remove this code.
             */
            using namespace std::literals;
            std::this_thread::sleep_for(160ms);
        }
    }

    if ((ip > ip_end) or (ip < ip_begin)) {
        std::cerr << "[vm] ip " << std::hex << std::setw(8) << std::setfill('0')
                  << ((ip - ip_begin) * sizeof(viua::arch::instruction_type))
                  << std::dec << " outside of valid range\n";
    }
}
}  // namespace

auto main(int argc, char* argv[]) -> int
{
    /*
     * If invoked with some operands, use the first of them as the
     * binary to load and execute. It most probably will be the sample
     * executable generated by an earlier invokation of the codec
     * testing program.
     */
    auto const executable_path = std::string{(argc > 1) ? argv[1] : "./a.out"};

    auto strings = std::vector<uint8_t>{};
    auto text    = std::vector<viua::arch::instruction_type>{};

    auto entry_addr = size_t{0};

    {
        auto const a_out = open(executable_path.c_str(), O_RDONLY);
        if (a_out == -1) {
            close(a_out);
            exit(1);
        }

        Elf64_Ehdr elf_header{};
        read(a_out, &elf_header, sizeof(elf_header));

        Elf64_Phdr program_header{};

        /*
         * We need to skip a few program headers which are just used to make
         * the file a proper ELF as recognised by file(1) and readelf(1).
         */
        read(a_out, &program_header, sizeof(program_header));  // skip magic
                                                               // PT_NULL
        read(a_out, &program_header, sizeof(program_header));  // skip PT_INTERP

        /*
         * Then come the actually useful program headers describing PT_LOAD
         * segments with .text section (containing the instructions we need to
         * run the program), and .rodata (containing the strings representing
         * strings and symbols).
         */
        Elf64_Phdr text_header{};
        read(a_out, &text_header, sizeof(text_header));

        Elf64_Phdr strings_header{};
        read(a_out, &strings_header, sizeof(strings_header));

        lseek(a_out, text_header.p_offset, SEEK_SET);
        text.resize(text_header.p_filesz
                    / sizeof(viua::arch::instruction_type));
        read(a_out, text.data(), text_header.p_filesz);

        if (strings_header.p_filesz) {
            lseek(a_out, strings_header.p_offset, SEEK_SET);
            strings.resize(strings_header.p_filesz);
            read(a_out, strings.data(), strings_header.p_filesz);
        }

        entry_addr = (elf_header.e_entry - text_header.p_offset)
                     / sizeof(viua::arch::instruction_type);

        std::cout << "[vm] loaded " << text_header.p_filesz
                  << " byte(s) of .text section from PT_LOAD segment of "
                  << executable_path << "\n";
        std::cout << "[vm] loaded "
                  << (text_header.p_filesz / sizeof(decltype(text)::value_type))
                  << " instructions\n";
        std::cout << "[vm] entry address at 0x" << std::hex << std::setw(8)
                  << std::setfill('0')
                  << (entry_addr * sizeof(viua::arch::instruction_type))
                  << std::dec << "\n";
        std::cout
            << "[vm] loaded " << strings_header.p_filesz
            << " byte(s) of .rodata (strings) section from PT_LOAD segment of "
            << executable_path << "\n";

        close(a_out);
    }

    auto registers = std::vector<Value>(256);

    auto const ip_begin = &text[0];
    auto const ip_end   = (ip_begin + text.size());
    run(registers,
        strings,
        (text.data() + entry_addr),
        {(executable_path + "[.text]"), ip_begin, ip_end});

    return 0;
}
