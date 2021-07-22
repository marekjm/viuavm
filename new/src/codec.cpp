#include <stdint.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <utility>


namespace machine::arch {
    using opcode_type = uint16_t;

    constexpr auto GREEDY = opcode_type{0x8000};
    constexpr auto FORMAT_N = opcode_type{0x0000};
    constexpr auto FORMAT_T = opcode_type{0x1000};
    constexpr auto FORMAT_D = opcode_type{0x2000};
    constexpr auto FORMAT_S = opcode_type{0x3000};
    constexpr auto FORMAT_F = opcode_type{0x4000};
    constexpr auto FORMAT_E = opcode_type{0x5000};
    constexpr auto FORMAT_R = opcode_type{0x6000};

    constexpr auto OPCODE_MASK = opcode_type{0x7fff};

    enum class Opcode : opcode_type {
        NOOP   = (FORMAT_N | 0x0000),
        EBREAK = (FORMAT_N | 0x0001),

        ADD    = (FORMAT_T | 0x0001),
        SUB    = (FORMAT_T | 0x0002),
        MUL    = (FORMAT_T | 0x0003),
        DIV    = (FORMAT_T | 0x0004),

        LUI    = (FORMAT_E | 0x0001),

        ADDI   = (FORMAT_R | 0x0001),
    };
    auto to_string(opcode_type const) -> std::string;

    enum class Register_set {
        /*
         * Void used as output register means that the value should be dropped.
         * Void used as input register means that a default value should be
         * used.
         */
        Void = 0,

        /*
         * Local and static variables.
         */
        Local,
        Static,

        /*
         * Parameters (ie, formal parameters) are declared and used by the
         * callee. Arguments (ie, actual parameters) are made by the caller.
         */
        Parameters,
        Arguments,
    };
    using Rs = Register_set;
}
namespace machine::arch {
    auto to_string(opcode_type const raw) -> std::string
    {
        static auto const OP_TO_NAME = std::map<Opcode, std::string>{
              { Opcode::NOOP, "noop" }
            , { Opcode::EBREAK, "ebreak" }

            , { Opcode::ADD, "add" }
            , { Opcode::SUB, "sub" }
            , { Opcode::MUL, "mul" }
            , { Opcode::DIV, "div" }

            , { Opcode::LUI, "lui" }
        };

        auto const greedy = static_cast<bool>(raw & GREEDY);
        auto const opcode = static_cast<Opcode>(raw & OPCODE_MASK);

        if (OP_TO_NAME.count(opcode)) {
            return (greedy ? "g." : "") + OP_TO_NAME.at(opcode);
        } else {
            return "<unknown>";
        }
    }
}

namespace codec::formats {
    struct Register_access {
        machine::arch::Register_set const set;
        bool const direct;
        uint8_t const index;

        Register_access();
        Register_access(machine::arch::Register_set const, bool const, uint8_t const);

        static auto decode(uint16_t const) -> Register_access;
        auto encode() const -> uint16_t;

        auto operator==(Register_access const& other) const -> bool
        {
            return (set == other.set) and (direct == other.direct) and (index == other.index);
        }

        inline auto is_legal() const -> bool
        {
            if (set == machine::arch::Register_set::Void and not direct and index != 0) {
                return false;
            }
            return true;
        }

        inline auto is_void() const -> bool
        {
            return (set == machine::arch::Register_set::Void);
        }
    };
    using Ra = Register_access;
    auto make_local_access(uint8_t const, bool const = true);

    /*
     * All instructions are encoded in a single encoding unit which is 64 bits
     * wide.
     */
    using eu_type = uint64_t;

    /*
     * Three-way (triple) register access.
     */
    struct T {
        machine::arch::opcode_type opcode;
        Register_access const out;
        Register_access const lhs;
        Register_access const rhs;

        T(
              machine::arch::opcode_type const
            , Register_access const
            , Register_access const
            , Register_access const
        );

        static auto decode(eu_type const) -> T;
        auto encode() const -> eu_type;
    };

    /*
     * Two-way (double) register access.
     */
    struct D {
        machine::arch::opcode_type opcode;
        Register_access const out;
        Register_access const in;

        D(
              machine::arch::opcode_type const
            , Register_access const
            , Register_access const
        );

        static auto decode(eu_type const) -> D;
        auto encode() const -> eu_type;
    };

    /*
     * One-way (single) register access.
     */
    struct S {
        machine::arch::opcode_type opcode;
        Register_access const out;

        S(
              machine::arch::opcode_type const
            , Register_access const
        );

        static auto decode(eu_type const) -> S;
        auto encode() const -> eu_type;
    };

    /*
     * One-way register access with 32-bit wide immediate value.
     * "F" because it is used for eg, floats.
     */
    struct F {
        machine::arch::opcode_type opcode;
        Register_access const out;
        uint32_t const immediate;

        F(
              machine::arch::opcode_type const op
            , Register_access const o
            , uint32_t const i
        );

        template<typename T>
        static auto make(
              machine::arch::opcode_type const op
            , Register_access const o
            , T const v
        ) -> F
        {
            static_assert(sizeof(T) == sizeof(uint32_t));
            auto imm = uint32_t{};
            memcpy(&imm, &v, sizeof(imm));
            return F{op, o, imm};
        }

        static auto decode(eu_type const) -> F;
        auto encode() const -> eu_type;
    };

    /*
     * One-way register access with 36-bit wide immediate value.
     * "E" because it is "extended" immediate, 4 bits longer than the F format.
     */
    struct E {
        machine::arch::opcode_type opcode;
        Register_access const out;
        uint64_t const immediate;

        E(
              machine::arch::opcode_type const op
            , Register_access const o
            , uint64_t const i
        );

        static auto decode(eu_type const) -> E;
        auto encode() const -> eu_type;
    };

    /*
     * Two-way register access with 24-bit wide immediate value.
     * "R" because it is "reduced" immediate, 8 bits shorter than the F format.
     */
    struct R {
        machine::arch::opcode_type opcode;
        Register_access const out;
        Register_access const in;
        uint32_t const immediate;

        R(
              machine::arch::opcode_type const
            , Register_access const
            , Register_access const
            , uint32_t const
        );

        static auto decode(eu_type const) -> R;
        auto encode() const -> eu_type;
    };
}

namespace codec::formats {
    Register_access::Register_access()
        : set{machine::arch::Register_set::Void}
        , direct{true}
        , index{0}
    {}
    Register_access::Register_access(machine::arch::Register_set const s, bool const d, uint8_t const i)
        : set{s}
        , direct{d}
        , index{i}
    {}
    auto Register_access::decode(uint16_t const raw) -> Register_access
    {
        auto set = static_cast<machine::arch::Register_set>((raw & 0x0e00) >> 9);
        auto direct = static_cast<bool>(raw & 0x0100);
        auto index = static_cast<uint8_t>(raw & 0x00ff);
        return Register_access{set, direct, index};
    }
    auto Register_access::encode() const -> uint16_t
    {
        auto base = uint16_t{index};
        auto mode = static_cast<uint16_t>(direct);
        auto rset = static_cast<uint16_t>(set);
        return base | (mode << 8) | (rset << 9);
    }

    auto make_local_access(uint8_t const index, bool const direct)
    {
        return Register_access{machine::arch::Rs::Local, direct, index};
    }
}
namespace codec::formats {
    T::T(
          machine::arch::opcode_type const op
        , Register_access const o
        , Register_access const l
        , Register_access const r
    )
        : opcode{op}
        , out{o}
        , lhs{l}
        , rhs{r}
    {}
    auto T::decode(eu_type const raw) -> T
    {
        auto opcode = static_cast<machine::arch::opcode_type>(raw & 0x000000000000ffff);
        auto out = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
        auto lhs = Register_access::decode((raw & 0x0000ffff00000000) >> 32);
        auto rhs = Register_access::decode((raw & 0xffff000000000000) >> 48);
        return T{opcode, out, lhs, rhs};
    }
    auto T::encode() const -> eu_type
    {
        auto base = uint64_t{opcode};
        auto output_register = uint64_t{out.encode()};
        auto left_hand_register = uint64_t{lhs.encode()};
        auto right_hand_register = uint64_t{rhs.encode()};
        return base
            | (output_register << 16)
            | (left_hand_register << 32)
            | (right_hand_register << 48);
    }
}
namespace codec::formats {
    D::D(
          machine::arch::opcode_type const op
        , Register_access const o
        , Register_access const i
    )
        : opcode{op}
        , out{o}
        , in{i}
    {}
    auto D::decode(eu_type const raw) -> D
    {
        auto opcode = static_cast<machine::arch::opcode_type>(raw & 0x000000000000ffff);
        auto out = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
        auto in = Register_access::decode((raw & 0x0000ffff00000000) >> 32);
        return D{opcode, out, in};
    }
    auto D::encode() const -> eu_type
    {
        auto base = uint64_t{opcode};
        auto output_register = uint64_t{out.encode()};
        auto input_register = uint64_t{in.encode()};
        return base
            | (output_register << 16)
            | (input_register << 32);
    }
}
namespace codec::formats {
    S::S(
          machine::arch::opcode_type const op
        , Register_access const o
    )
        : opcode{op}
        , out{o}
    {}
    auto S::decode(eu_type const raw) -> S
    {
        auto opcode = static_cast<machine::arch::opcode_type>(raw & 0x000000000000ffff);
        auto out = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
        return S{opcode, out};
    }
    auto S::encode() const -> eu_type
    {
        auto base = uint64_t{opcode};
        auto output_register = uint64_t{out.encode()};
        return base | (output_register << 16);
    }
}
namespace codec::formats {
    F::F(
          machine::arch::opcode_type const op
        , Register_access const o
        , uint32_t const i
    )
        : opcode{op}
        , out{o}
        , immediate{i}
    {}
    auto F::decode(eu_type const raw) -> F
    {
        auto opcode = static_cast<machine::arch::opcode_type>(raw & 0x000000000000ffff);
        auto out = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
        auto value = static_cast<uint32_t>(raw >> 32);
        return F{opcode, out, value};
    }
    auto F::encode() const -> eu_type
    {
        auto base = uint64_t{opcode};
        auto output_register = uint64_t{out.encode()};
        auto value = uint64_t{immediate};
        return base | (output_register << 16) | (value << 32);
    }
}
namespace codec::formats {
    E::E(
          machine::arch::opcode_type const op
        , Register_access const o
        , uint64_t const i
    )
        : opcode{op}
        , out{o}
        , immediate{i}
    {}
    auto E::decode(eu_type const raw) -> E
    {
        auto const opcode = static_cast<machine::arch::opcode_type>(raw & 0x000000000000ffff);
        auto const out = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
        auto const high = (((raw >> 28) & 0xf) << 32);
        auto const low = ((raw >> 32) & 0x00000000ffffffff);
        auto const value = (high | low);
        return E{opcode, out, value};
    }
    auto E::encode() const -> eu_type
    {
        auto base = uint64_t{opcode};
        auto output_register = uint64_t{out.encode()};
        auto high = ((immediate & 0x0000000f00000000) >> 32);
        auto low = (immediate & 0x00000000ffffffff);
        return base
            | (output_register << 16)
            | (high << 28)
            | (low << 32);
    }
}
namespace codec::formats {
    R::R(
          machine::arch::opcode_type const op
        , Register_access const o
        , Register_access const i
        , uint32_t const im
    )
        : opcode{op}
        , out{o}
        , in{i}
        , immediate{im}
    {}
    auto R::decode(eu_type const raw) -> R
    {
        auto const opcode = static_cast<machine::arch::opcode_type>(raw & 0x000000000000ffff);
        auto const out = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
        auto const in = Register_access::decode((raw & 0x0000ffff00000000) >> 32);

        auto const low_short   = static_cast<uint32_t>((raw & 0xffff000000000000) >> 48);
        auto const low_nibble  = static_cast<uint32_t>((raw & 0x0000f00000000000) >> 44);
        auto const high_nibble = static_cast<uint32_t>((raw & 0x00000000f0000000) >> 28);

        auto const value = low_short | (low_nibble << 16) | (high_nibble << 20);

        return R{opcode, out, in, value};
    }
    auto R::encode() const -> eu_type
    {
        auto base = uint64_t{opcode};
        auto output_register = uint64_t{out.encode()};
        auto input_register = uint64_t{in.encode()};

        auto const high_nibble = uint64_t{(immediate & 0x00f00000) >> 20};
        auto const low_nibble  = uint64_t{(immediate & 0x000f0000) >> 16};
        auto const low_short   = uint64_t{immediate & 0x0000ffff};

        return base
            | (output_register << 16)
            | (input_register << 32)
            | (low_short << 48)
            | (high_nibble << 28)
            | (low_nibble << 44);
    }
}

namespace machine::ops {
    struct Op {};

    struct NOOP : Op {};
    struct EBREAK : Op {};

    struct ADD : Op {
        codec::formats::T instruction;
    };
    struct SUB : Op {
        codec::formats::T instruction;
    };
    struct MUL : Op {
        codec::formats::T instruction;
    };
    struct DIV : Op {
        codec::formats::T instruction;
    };

    struct LUI : Op {
        codec::formats::E instruction;

        LUI(codec::formats::E i): instruction{i} {}
    };

    struct ADDIU : Op {
        codec::formats::R instruction;

        ADDIU(codec::formats::R i): instruction{i} {}
    };
}

auto to_loading_parts_unsigned(uint64_t const value) -> std::pair<uint64_t, std::vector<uint32_t>>
{
    constexpr auto LOW_24 = uint64_t{0x0000000000ffffff};
    constexpr auto HIGH_36 = uint64_t{0xfffffffff0000000};

    auto const high_part = ((value & HIGH_36) >> 28);
    auto const low_part = static_cast<uint32_t>(value & ~HIGH_36);

    /*
     * If the low part consists of only 24 bits we can use two instead of three
     * instructions:
     *
     *  1/ lui to load high 36 bits
     *  2/ addi to add low 24 bits
     *
     * This saves one instructions pushing the overhead down.
     */
    if ((low_part & LOW_24) == low_part) {
        return { high_part, { low_part } };
    }

    auto addis = std::vector<uint32_t>{};
    auto const is_odd = static_cast<bool>(low_part % 2);
    addis.push_back((low_part / 2) + is_odd);
    addis.push_back(low_part / 2);

    return { high_part, addis };
}

struct Value {
    bool boxed { false };
    std::variant<uint64_t, void*> value;
};

namespace {
    auto execute(std::vector<Value>& registers, machine::ops::LUI const op) -> void
    {
        auto& value = registers.at(op.instruction.out.index);
        value.value = (op.instruction.immediate << 28);
    }
    auto execute(std::vector<Value>& registers, machine::ops::ADDIU const op) -> void
    {
        auto& value = registers.at(op.instruction.out.index);
        value.value = (std::get<uint64_t>(value.value) + op.instruction.immediate);
    }
    auto op_li(std::vector<Value>& registers, uint64_t const value) -> void
    {
        auto const parts = to_loading_parts_unsigned(value);

        execute(registers, machine::ops::LUI{codec::formats::E{
            (machine::arch::GREEDY
             | static_cast<machine::arch::opcode_type>(machine::arch::Opcode::LUI))
            , codec::formats::make_local_access(1)
            , parts.first
        }});
        for (auto const each : parts.second) {
            // FIXME make all but last instruction greedy
            execute(registers, machine::ops::ADDIU{codec::formats::R{
                  static_cast<machine::arch::opcode_type>(machine::arch::Opcode::LUI)
                , codec::formats::make_local_access(1)
                , codec::formats::make_local_access(1)
                , each
            }});
        }
    }
}

auto main() -> int
{
    {
        auto const tm = codec::formats::T{
              0xdead
            , codec::formats::Register_access{machine::arch::Register_set::Local, true, 0xff}
            , codec::formats::Register_access{machine::arch::Register_set::Local, true, 0x01}
            , codec::formats::Register_access{machine::arch::Register_set::Local, true, 0x02}
        };
        std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
        auto const td = codec::formats::T::decode(tm.encode());
        std::cout
            << (tm.opcode == td.opcode)
            << (tm.out == td.out)
            << (tm.lhs == td.lhs)
            << (tm.rhs == td.rhs)
            << "\n";
    }
    {
        auto const tm = codec::formats::D{
              0xdead
            , codec::formats::Register_access{machine::arch::Register_set::Local, true, 0xff}
            , codec::formats::Register_access{machine::arch::Register_set::Local, true, 0x01}
        };
        std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
        auto const td = codec::formats::D::decode(tm.encode());
        std::cout
            << (tm.opcode == td.opcode)
            << (tm.out == td.out)
            << (tm.in == td.in)
            << "\n";
    }
    {
        auto const tm = codec::formats::S{
              0xdead
            , codec::formats::Register_access{machine::arch::Register_set::Local, true, 0xff}
        };
        std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
        auto const td = codec::formats::S::decode(tm.encode());
        std::cout
            << (tm.opcode == td.opcode)
            << (tm.out == td.out)
            << "\n";
    }
    {
        constexpr auto original_value = 3.14f;

        auto imm_in = uint32_t{};
        memcpy(&imm_in, &original_value, sizeof(imm_in));

        auto const tm = codec::formats::F{
              0xdead
            , codec::formats::Register_access{machine::arch::Register_set::Local, true, 0xff}
            , imm_in
        };
        std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
        auto const td = codec::formats::F::decode(tm.encode());

        auto imm_out = float{};
        memcpy(&imm_out, &td.immediate, sizeof(imm_out));

        std::cout
            << (tm.opcode == td.opcode)
            << (tm.out == td.out)
            << (tm.immediate == td.immediate)
            << (imm_out == original_value)
            << "\n";
    }
    {
        auto const tm = codec::formats::E{
              0xdead
            , codec::formats::Register_access{machine::arch::Register_set::Local, true, 0xff}
            , 0xabcdef012
        };
        std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
        auto const td = codec::formats::E::decode(tm.encode());
        std::cout
            << (tm.opcode == td.opcode)
            << (tm.out == td.out)
            << (tm.immediate == td.immediate)
            << "\n";
    }
    {
        auto const tm = codec::formats::R{
              0xdead
            , codec::formats::Register_access{machine::arch::Register_set::Local, true, 0x55}
            , codec::formats::Register_access{machine::arch::Register_set::Local, true, 0x22}
            , 0xabcdef
        };
        std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
        auto const td = codec::formats::R::decode(tm.encode());
        std::cout
            << (tm.opcode == td.opcode)
            << (tm.out == td.out)
            << (tm.in == td.in)
            << (tm.immediate == td.immediate)
            << "\n";
    }

    if constexpr (false) {
        auto const test_these = std::vector<uint64_t>{
              0x0000000000000000
            , 0x0000000000000001
            , 0x0000000000bedead /* low 24 */
            , 0x00000000deadbeef /* low 32 */
            , 0xdeadbeefd0adbeef /* high 36 and low 24 (special case) */
            , 0xdeadbeefd1adbeef /* all bits */
            , 0xdeadbeefd2adbeef /* all bits */
            , 0xdeadbeefd3adbeef /* all bits */
            , 0xdeadbeefd4adbeef /* all bits */
            , 0xdeadbeefd5adbeef /* all bits */
            , 0xdeadbeefd6adbeef /* all bits */
            , 0xdeadbeefd7adbeef /* all bits */
            , 0xdeadbeefd8adbeef /* all bits */
            , 0xdeadbeefd9adbeef /* all bits */
            , 0xdeadbeefdaadbeef /* all bits */
            , 0xdeadbeefdbadbeef /* all bits */
            , 0xdeadbeefdcadbeef /* all bits */
            , 0xdeadbeefddadbeef /* all bits */
            , 0xdeadbeefdeadbeef /* all bits */
            , 0xdeadbeeffdadbeef /* all bits */
            , 0xffffffffffffffff
        };

        for (auto const wanted : test_these) {
            std::cout << "\n";

            auto const parts = to_loading_parts_unsigned(wanted);

            auto got = (parts.first << 28);
            /* std::cout << std::hex; */
            /* std::cout << "  lui   ..., 0x" << parts.first << "\n"; */
            for (auto const each : parts.second) {
                /* std::cout << "  addiu ..., 0x" << each << "\n"; */
                got += each;
            }

            std::cout << std::hex << std::setw(16) << std::setfill('0') << wanted << "\n";
            std::cout << std::hex << std::setw(16) << std::setfill('0') << got << "\n";
            std::cout << "  cost: " << std::dec
                << (parts.second.size() + (parts.first ? 1 : 0)) << "\n";
            if (wanted != got) {
                std::cerr << "BAD BAD BAD!\n";
                break;
            }
        }
    }

    std::cout << machine::arch::to_string(0x0000) << "\n";
    std::cout << machine::arch::to_string(0x0001) << "\n";
    std::cout << machine::arch::to_string(0x1001) << "\n";
    std::cout << machine::arch::to_string(0x9001) << "\n";
    std::cout << machine::arch::to_string(0x1002) << "\n";
    std::cout << machine::arch::to_string(0x1003) << "\n";
    std::cout << machine::arch::to_string(0x1004) << "\n";
    std::cout << machine::arch::to_string(0x5001) << "\n";

    {
        auto registers = std::vector<Value>(256);

        op_li(registers, 0xdeadbeefdeadbeef);

        std::cout << std::hex << std::setw(16) << std::setfill('0')
            << std::get<uint64_t>(registers.at(1).value) << "\n";
    }

    return 0;
}
