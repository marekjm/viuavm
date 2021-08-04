#include <viua/arch/arch.h>
#include <viua/arch/ops.h>

#include <iostream>
#include <iomanip>
#include <chrono>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <utility>
#include <thread>
#include <type_traits>

#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>


namespace codec::formats {
    struct Register_access {
        using rs_type = viua::arch::REGISTER_SET;

        rs_type const set;
        bool const direct;
        uint8_t const index;

        Register_access();
        Register_access(rs_type const, bool const, uint8_t const);

        static auto decode(uint16_t const) -> Register_access;
        auto encode() const -> uint16_t;

        auto operator==(Register_access const& other) const -> bool
        {
            return (set == other.set) and (direct == other.direct) and (index == other.index);
        }

        inline auto is_legal() const -> bool
        {
            if (is_void() and not direct and index != 0) {
                return false;
            }
            return true;
        }

        inline auto is_void() const -> bool
        {
            return (set == rs_type::VOID);
        }
    };
    using Ra = Register_access;
    auto make_local_access(uint8_t const, bool const = true);
    auto make_void_access();

    /*
     * All instructions are encoded in a single encoding unit which is 64 bits
     * wide.
     */
    using eu_type = uint64_t;

    /*
     * Three-way (triple) register access.
     */
    struct T {
        viua::arch::opcode_type opcode;
        Register_access const out;
        Register_access const lhs;
        Register_access const rhs;

        T(
              viua::arch::opcode_type const
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
        viua::arch::opcode_type opcode;
        Register_access const out;
        Register_access const in;

        D(
              viua::arch::opcode_type const
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
        viua::arch::opcode_type opcode;
        Register_access const out;

        S(
              viua::arch::opcode_type const
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
        viua::arch::opcode_type opcode;
        Register_access const out;
        uint32_t const immediate;

        F(
              viua::arch::opcode_type const op
            , Register_access const o
            , uint32_t const i
        );

        template<typename T>
        static auto make(
              viua::arch::opcode_type const op
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
        viua::arch::opcode_type opcode;
        Register_access const out;
        uint64_t const immediate;

        E(
              viua::arch::opcode_type const op
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
        viua::arch::opcode_type opcode;
        Register_access const out;
        Register_access const in;
        uint32_t const immediate;

        R(
              viua::arch::opcode_type const
            , Register_access const
            , Register_access const
            , uint32_t const
        );

        static auto decode(eu_type const) -> R;
        auto encode() const -> eu_type;
    };

    /*
     * No operands.
     */
    struct N {
        viua::arch::opcode_type opcode;

        N(
              viua::arch::opcode_type const
        );

        static auto decode(eu_type const) -> N;
        auto encode() const -> eu_type;
    };
}

namespace codec::formats {
    Register_access::Register_access()
        : set{viua::arch::REGISTER_SET::VOID}
        , direct{true}
        , index{0}
    {}
    Register_access::Register_access(viua::arch::REGISTER_SET const s, bool const d, uint8_t const i)
        : set{s}
        , direct{d}
        , index{i}
    {}
    auto Register_access::decode(uint16_t const raw) -> Register_access
    {
        auto set = static_cast<viua::arch::REGISTER_SET>((raw & 0x0e00) >> 9);
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
        return Register_access{viua::arch::REGISTER_SET::LOCAL, direct, index};
    }
    auto make_void_access()
    {
        return Register_access{viua::arch::REGISTER_SET::VOID, true, 0};
    }
}
namespace codec::formats {
    T::T(
          viua::arch::opcode_type const op
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
        auto opcode = static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
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
          viua::arch::opcode_type const op
        , Register_access const o
        , Register_access const i
    )
        : opcode{op}
        , out{o}
        , in{i}
    {}
    auto D::decode(eu_type const raw) -> D
    {
        auto opcode = static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
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
          viua::arch::opcode_type const op
        , Register_access const o
    )
        : opcode{op}
        , out{o}
    {}
    auto S::decode(eu_type const raw) -> S
    {
        auto opcode = static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
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
          viua::arch::opcode_type const op
        , Register_access const o
        , uint32_t const i
    )
        : opcode{op}
        , out{o}
        , immediate{i}
    {}
    auto F::decode(eu_type const raw) -> F
    {
        auto opcode = static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
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
          viua::arch::opcode_type const op
        , Register_access const o
        , uint64_t const i
    )
        : opcode{op}
        , out{o}
        , immediate{i}
    {}
    auto E::decode(eu_type const raw) -> E
    {
        auto const opcode = static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
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
          viua::arch::opcode_type const op
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
        auto const opcode = static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);
        auto const out = Register_access::decode((raw & 0x00000000ffff0000) >> 16);
        auto const in = Register_access::decode((raw & 0x0000ffff00000000) >> 32);

        auto const low_short   = static_cast<uint32_t>((raw & 0xffff000000000000) >> 48);
        auto const low_nibble  = static_cast<uint32_t>((raw & 0x0000f00000000000) >> 44);
        auto const high_nibble = static_cast<uint32_t>((raw & 0x00000000f0000000) >> 28);

        auto const immediate = low_short | (low_nibble << 16) | (high_nibble << 20);

        return R{opcode, out, in, immediate};
    }
    auto R::encode() const -> eu_type
    {
        auto base = uint64_t{opcode};
        auto output_register = uint64_t{out.encode()};
        auto input_register = uint64_t{in.encode()};

        auto const high_nibble = uint64_t{(immediate & 0x00f00000) >> 20};
        auto const low_nibble  = uint64_t{(immediate & 0x000f0000) >> 16};
        auto const low_short   = uint64_t{(immediate & 0x0000ffff) >> 0};

        return base
            | (output_register << 16)
            | (input_register  << 32)
            | (low_short       << 48)
            | (high_nibble     << 28)
            | (low_nibble      << 44);
    }
}
namespace codec::formats {
    N::N(
          viua::arch::opcode_type const op
    )
        : opcode{op}
    {}
    auto N::decode(eu_type const raw) -> N
    {
        auto const opcode = static_cast<viua::arch::opcode_type>(raw & 0x000000000000ffff);

        return N{opcode};
    }
    auto N::encode() const -> eu_type
    {
        return uint64_t{opcode};
    }
}

namespace viua::arch::ops {
    struct Op {};

    struct NOOP : Op {};
    struct EBREAK : Op {
        codec::formats::N instruction;

        EBREAK(codec::formats::N i): instruction{i} {}
    };

    struct ADD : Op {
        codec::formats::T instruction;

        ADD(codec::formats::T i): instruction{i} {}
    };
    struct SUB : Op {
        codec::formats::T instruction;
    };
    struct MUL : Op {
        codec::formats::T instruction;

        MUL(codec::formats::T i): instruction{i} {}
    };
    struct DIV : Op {
        codec::formats::T instruction;
    };
    struct MOD : Op {
        codec::formats::T instruction;
    };

    /*
     * DELETE clears a register and deletes a value that it contained. For
     * unboxed values the bit-pattern is simply deleted; for boxed values their
     * destructor is invoked.
     */
    struct DELETE : Op {
        codec::formats::S instruction;

        DELETE(codec::formats::S i): instruction{i} {}
    };

    /*
     * LUI loads upper bits of a 64-bit value, sign-extending it to register
     * width. It produces a signed integer.
     *
     * LUIU is the unsigned version of LUI and does not perform sign-extension.
     */
    struct LUI : Op {
        codec::formats::E instruction;

        LUI(codec::formats::E i): instruction{i} {}
    };
    struct LUIU : Op {
        codec::formats::E instruction;

        LUIU(codec::formats::E i): instruction{i} {}
    };

    /*
     * ADDIU adds 24-bit immediate unsigned integer as right-hand operand, to a
     * left-hand operand taken from a register. The left-hand operand is
     * converted to an unsigned integer, and the value produced is an unsigned
     * integer.
     */
    struct ADDIU : Op {
        codec::formats::R instruction;

        ADDIU(codec::formats::R i): instruction{i} {}
    };
    struct ADDI : Op {
        codec::formats::R instruction;

        ADDI(codec::formats::R i): instruction{i} {}
    };
}

auto to_loading_parts_unsigned(uint64_t const value)
    -> std::pair<uint64_t, std::pair<std::pair<uint32_t, uint32_t>, uint32_t>>
{
    constexpr auto LOW_24  = uint64_t{0x0000000000ffffff};
    constexpr auto HIGH_36 = uint64_t{0xfffffffff0000000};

    auto const high_part = ((value & HIGH_36) >> 28);
    auto const low_part = static_cast<uint32_t>(value & ~HIGH_36);

    /*
     * If the low part consists of only 24 bits we can use just two
     * instructions:
     *
     *  1/ lui to load high 36 bits
     *  2/ addi to add low 24 bits
     *
     * This reduces the overhead of loading 64-bit values.
     */
    if ((low_part & LOW_24) == low_part) {
        return { high_part, { { low_part, 0 }, 0 } };
    }

    auto const multiplier = 16;
    auto const remainder = (low_part % multiplier);
    auto const base = (low_part - remainder) / multiplier;

    return { high_part, { { base, multiplier }, remainder } };
}

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
    std::variant<uint64_t, void*> value;

    auto is_boxed() const -> bool
    {
        return std::holds_alternative<void*>(value);
    }
    auto is_void() const -> bool
    {
        return ((not is_boxed()) and type_of_unboxed == Value::Unboxed_type::Void);
    }
};

namespace machine::core::ops {
    auto execute(std::vector<Value>& registers, viua::arch::ops::ADD const op) -> void
    {
        auto& out = registers.at(op.instruction.out.index);
        auto& lhs = registers.at(op.instruction.lhs.index);
        auto& rhs = registers.at(op.instruction.rhs.index);

        out.type_of_unboxed = lhs.type_of_unboxed;
        out.value = (std::get<uint64_t>(lhs.value) + std::get<uint64_t>(rhs.value));

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.lhs.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.rhs.index))
            + "\n";
    }
    auto execute(std::vector<Value>& registers, viua::arch::ops::SUB const op) -> void
    {
        auto& out = registers.at(op.instruction.out.index);
        auto& lhs = registers.at(op.instruction.lhs.index);
        auto& rhs = registers.at(op.instruction.rhs.index);

        out.type_of_unboxed = lhs.type_of_unboxed;
        out.value = (std::get<uint64_t>(lhs.value) - std::get<uint64_t>(rhs.value));

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.lhs.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.rhs.index))
            + "\n";
    }
    auto execute(std::vector<Value>& registers, viua::arch::ops::MUL const op) -> void
    {
        auto& out = registers.at(op.instruction.out.index);
        auto& lhs = registers.at(op.instruction.lhs.index);
        auto& rhs = registers.at(op.instruction.rhs.index);

        out.type_of_unboxed = lhs.type_of_unboxed;
        out.value = (std::get<uint64_t>(lhs.value) * std::get<uint64_t>(rhs.value));

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.lhs.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.rhs.index))
            + "\n";
    }
    auto execute(std::vector<Value>& registers, viua::arch::ops::DIV const op) -> void
    {
        auto& out = registers.at(op.instruction.out.index);
        auto& lhs = registers.at(op.instruction.lhs.index);
        auto& rhs = registers.at(op.instruction.rhs.index);

        out.type_of_unboxed = lhs.type_of_unboxed;
        out.value = (std::get<uint64_t>(lhs.value) / std::get<uint64_t>(rhs.value));

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.lhs.index))
            + ", %" + std::to_string(static_cast<int>(op.instruction.rhs.index))
            + "\n";
    }

    auto execute(std::vector<Value>& registers, viua::arch::ops::DELETE const op) -> void
    {
        auto& target = registers.at(op.instruction.out.index);

        target.type_of_unboxed = Value::Unboxed_type::Void;
        target.value = uint64_t{0};

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + ", %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + "\n";
    }

    auto execute(std::vector<Value>& registers, viua::arch::ops::LUI const op) -> void
    {
        auto& value = registers.at(op.instruction.out.index);
        value.type_of_unboxed = Value::Unboxed_type::Integer_signed;
        value.value = (op.instruction.immediate << 28);

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", " + std::to_string(op.instruction.immediate)
            + "\n";
    }
    auto execute(std::vector<Value>& registers, viua::arch::ops::LUIU const op) -> void
    {
        auto& value = registers.at(op.instruction.out.index);
        value.type_of_unboxed = Value::Unboxed_type::Integer_unsigned;
        value.value = (op.instruction.immediate << 28);

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", " + std::to_string(op.instruction.immediate)
            + "\n";
    }

    auto execute(std::vector<Value>& registers, viua::arch::ops::ADDI const op) -> void
    {
        auto& out = registers.at(op.instruction.out.index);
        auto const base = (op.instruction.in.is_void()
            ? 0
            : std::get<uint64_t>(registers.at(op.instruction.in.index).value));

        out.type_of_unboxed = Value::Unboxed_type::Integer_signed;
        out.value = (base + op.instruction.immediate);

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", void" // FIXME it's not always void
            + ", " + std::to_string(op.instruction.immediate)
            + "\n";
    }
    auto execute(std::vector<Value>& registers, viua::arch::ops::ADDIU const op) -> void
    {
        auto& out = registers.at(op.instruction.out.index);
        auto const base = (op.instruction.in.is_void()
            ? 0
            : std::get<uint64_t>(registers.at(op.instruction.in.index).value));

        out.type_of_unboxed = Value::Unboxed_type::Integer_unsigned;
        out.value = (base + op.instruction.immediate);

        std::cerr << "    " + viua::arch::ops::to_string(op.instruction.opcode)
            + " %" + std::to_string(static_cast<int>(op.instruction.out.index))
            + ", void" // FIXME it's not always void
            + ", " + std::to_string(op.instruction.immediate)
            + "\n";
    }

    auto execute(std::vector<Value>& registers, viua::arch::ops::EBREAK const) -> void
    {
        for (auto i = size_t{0}; i < registers.size(); ++i) {
            auto const& each = registers.at(i);
            if (each.is_void()) {
                continue;
            }

            std::cerr << "[" << std::setw(3) << i << "] ";

            if (each.is_boxed()) {
                std::cerr << "<boxed>\n";
                continue;
            }

            switch (each.type_of_unboxed) {
                case Value::Unboxed_type::Void:
                    break;
                case Value::Unboxed_type::Byte:
                    std::cerr
                        << "by "
                        << std::hex
                        << std::setw(2)
                        << std::setfill('0')
                        << static_cast<uint8_t>(std::get<uint64_t>(each.value))
                        << "\n";
                    break;
                case Value::Unboxed_type::Integer_signed:
                    std::cerr
                        << "is "
                        << std::hex
                        << std::setw(16)
                        << std::setfill('0')
                        << std::get<uint64_t>(each.value)
                        << " "
                        << std::dec
                        << static_cast<int64_t>(std::get<uint64_t>(each.value))
                        << "\n";
                    break;
                case Value::Unboxed_type::Integer_unsigned:
                    std::cerr
                        << "iu "
                        << std::hex
                        << std::setw(16)
                        << std::setfill('0')
                        << std::get<uint64_t>(each.value)
                        << " "
                        << std::dec
                        << std::get<uint64_t>(each.value)
                        << "\n";
                    break;
                case Value::Unboxed_type::Float_single:
                    std::cerr
                        << "fl "
                        << std::hex
                        << std::setw(8)
                        << std::setfill('0')
                        << static_cast<float>(std::get<uint64_t>(each.value))
                        << " "
                        << std::dec
                        << static_cast<float>(std::get<uint64_t>(each.value))
                        << "\n";
                    break;
                case Value::Unboxed_type::Float_double:
                    std::cerr
                        << "db "
                        << std::hex
                        << std::setw(16)
                        << std::setfill('0')
                        << static_cast<double>(std::get<uint64_t>(each.value))
                        << " "
                        << std::dec
                        << static_cast<double>(std::get<uint64_t>(each.value))
                        << "\n";
                    break;
            }
        }
    }

    auto execute(std::vector<Value>& registers, viua::arch::instruction_type const* const ip)
        -> viua::arch::instruction_type const*
    {
        auto const raw = *ip;

        auto const opcode = static_cast<viua::arch::opcode_type>(raw & viua::arch::ops::OPCODE_MASK);
        auto const format = static_cast<viua::arch::ops::FORMAT>(opcode & viua::arch::ops::FORMAT_MASK);

        switch (format) {
            case viua::arch::ops::FORMAT::T:
            {
                auto instruction = codec::formats::T::decode(raw);
                switch (static_cast<viua::arch::ops::OPCODE_T>(opcode)) {
                    case viua::arch::ops::OPCODE_T::ADD:
                        execute(registers, viua::arch::ops::ADD{instruction});
                        break;
                    case viua::arch::ops::OPCODE_T::MUL:
                        execute(registers, viua::arch::ops::MUL{instruction});
                        break;
                    default:
                        std::cerr << "unimplemented T instruction\n";
                        return nullptr;
                }
                break;
            }
            case viua::arch::ops::FORMAT::S:
            {
                auto instruction = codec::formats::S::decode(raw);
                switch (static_cast<viua::arch::ops::OPCODE_S>(opcode)) {
                    case viua::arch::ops::OPCODE_S::DELETE:
                        execute(registers, viua::arch::ops::DELETE{instruction});
                        break;
                    default:
                        std::cerr << "unimplemented S instruction\n";
                        return nullptr;
                }
                break;
            }
            case viua::arch::ops::FORMAT::E:
            {
                auto instruction = codec::formats::E::decode(raw);
                switch (static_cast<viua::arch::ops::OPCODE_E>(opcode)) {
                    case viua::arch::ops::OPCODE_E::LUI:
                        execute(registers, viua::arch::ops::LUI{instruction});
                        break;
                    case viua::arch::ops::OPCODE_E::LUIU:
                        execute(registers, viua::arch::ops::LUIU{instruction});
                        break;
                }
                break;
            }
            case viua::arch::ops::FORMAT::R:
            {
                auto instruction = codec::formats::R::decode(raw);
                switch (static_cast<viua::arch::ops::OPCODE_R>(opcode)) {
                    case viua::arch::ops::OPCODE_R::ADDI:
                        execute(registers, viua::arch::ops::ADDI{instruction});
                        break;
                    case viua::arch::ops::OPCODE_R::ADDIU:
                        execute(registers, viua::arch::ops::ADDIU{instruction});
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
                        execute(registers, viua::arch::ops::EBREAK{
                            codec::formats::N::decode(raw)});
                        break;
                }
                break;
            }
            case viua::arch::ops::FORMAT::D:
            case viua::arch::ops::FORMAT::F:
                std::cerr << "unimplemented instruction: "
                    << viua::arch::ops::to_string(opcode)
                    << "\n";
                return nullptr;
        }

        return (ip + 1);
    }
}

namespace {
    auto op_li(uint64_t* instructions, uint64_t const value) -> uint64_t*
    {
        auto const parts = to_loading_parts_unsigned(value);

        /*
         * Only use the lui instruction of there's a reason to ie, if some of
         * the highest 36 bits are set. Otherwise, the lui is just overhead.
         */
        if (parts.first) {
            *instructions++ = codec::formats::E{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::LUIU))
                , codec::formats::make_local_access(1)
                , parts.first
            }.encode();
        }

        auto const base = parts.second.first.first;
        auto const multiplier = parts.second.first.second;

        if (multiplier != 0) {
            *instructions++ = codec::formats::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDIU))
                , codec::formats::make_local_access(2)
                , codec::formats::make_void_access()
                , base
            }.encode();
            *instructions++ = codec::formats::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDIU))
                , codec::formats::make_local_access(3)
                , codec::formats::make_void_access()
                , multiplier
            }.encode();
            *instructions++ = codec::formats::T{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::MUL))
                , codec::formats::make_local_access(2)
                , codec::formats::make_local_access(2)
                , codec::formats::make_local_access(3)
            }.encode();

            auto const remainder = parts.second.second;
            *instructions++ = codec::formats::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDIU))
                , codec::formats::make_local_access(3)
                , codec::formats::make_void_access()
                , remainder
            }.encode();
            *instructions++ = codec::formats::T{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADD))
                , codec::formats::make_local_access(2)
                , codec::formats::make_local_access(2)
                , codec::formats::make_local_access(3)
            }.encode();

            *instructions++ = codec::formats::T{
                 static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADD)
                , codec::formats::make_local_access(1)
                , codec::formats::make_local_access(1)
                , codec::formats::make_local_access(2)
            }.encode();
        } else {
            *instructions++ = codec::formats::R{
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDIU)
                , codec::formats::make_local_access(1)
                , codec::formats::make_void_access()
                , base
            }.encode();
        }

        return instructions;
    }
    auto op_li(uint64_t* instructions, int64_t const value) -> uint64_t*
    {
        auto const parts = to_loading_parts_unsigned(value);

        /*
         * Only use the lui instruction of there's a reason to ie, if some of
         * the highest 36 bits are set. Otherwise, the lui is just overhead.
         */
        if (parts.first) {
            *instructions++ = codec::formats::E{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::LUI))
                , codec::formats::make_local_access(1)
                , parts.first
            }.encode();
        }

        auto const base = parts.second.first.first;
        auto const multiplier = parts.second.first.second;

        if (multiplier != 0) {
            *instructions++ = codec::formats::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDI))
                , codec::formats::make_local_access(2)
                , codec::formats::make_void_access()
                , base
            }.encode();
            *instructions++ = codec::formats::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDI))
                , codec::formats::make_local_access(3)
                , codec::formats::make_void_access()
                , multiplier
            }.encode();
            *instructions++ = codec::formats::T{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::MUL))
                , codec::formats::make_local_access(2)
                , codec::formats::make_local_access(2)
                , codec::formats::make_local_access(3)
            }.encode();

            auto const remainder = parts.second.second;
            *instructions++ = codec::formats::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDI))
                , codec::formats::make_local_access(3)
                , codec::formats::make_void_access()
                , remainder
            }.encode();
            *instructions++ = codec::formats::T{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADD))
                , codec::formats::make_local_access(2)
                , codec::formats::make_local_access(2)
                , codec::formats::make_local_access(3)
            }.encode();

            *instructions++ = codec::formats::T{
                 static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADD)
                , codec::formats::make_local_access(1)
                , codec::formats::make_local_access(1)
                , codec::formats::make_local_access(2)
            }.encode();
        } else {
            *instructions++ = codec::formats::R{
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDI)
                , codec::formats::make_local_access(1)
                , codec::formats::make_void_access()
                , base
            }.encode();
        }

        return instructions;
    }

    auto run_instruction(std::vector<Value>& registers, uint64_t const* ip) -> uint64_t const*
    {
        auto instruction = uint64_t{};
        do {
            instruction = *ip;
            ip = machine::core::ops::execute(registers, ip);
        } while ((ip != nullptr) and (instruction & viua::arch::ops::GREEDY));

        return ip;
    }

    auto run(
          std::vector<Value>& registers
        , uint64_t const* ip
        , std::tuple<std::string_view const, uint64_t const*, uint64_t const*> ip_range
    ) -> void
    {
        auto const [ module, ip_begin, ip_end ] = ip_range;

        constexpr auto PREEMPTION_THRESHOLD = size_t{2};

        while (ip != ip_end) {
            auto const ip_before = ip;

            std::cerr << "cycle at " << module << "+0x"
                << std::hex << std::setw(8) << std::setfill('0')
                << ((ip - ip_begin) * sizeof(viua::arch::instruction_type))
                << std::dec << "\n";

            for (auto i = size_t{0}; i < PREEMPTION_THRESHOLD and ip != ip_end; ++i) {
                /*
                 * This is needed to detect greedy bundles and adjust preemption
                 * counter appropriately. If a greedy bundle contains more
                 * instructions than the preemption threshold allows the process
                 * will be suspended immediately.
                 */
                auto const greedy = (*ip & viua::arch::ops::GREEDY);
                auto const bundle_ip = ip;

                std::cerr << "  "
                    << (greedy ? "bundle" : "single")
                    << " "
                    << std::hex << std::setw(2) << std::setfill('0')
                    << i << std::dec << "\n";

                ip = run_instruction(registers, ip);

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
    }
}

auto main(int argc, char* argv[]) -> int
{
    if constexpr (false) {
        {
            auto const tm = codec::formats::T{
                  0xdead
                , codec::formats::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
                , codec::formats::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x01}
                , codec::formats::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x02}
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
                , codec::formats::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
                , codec::formats::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x01}
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
                , codec::formats::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
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
                , codec::formats::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
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
                , codec::formats::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
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
                , codec::formats::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x55}
                , codec::formats::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x22}
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

            auto high = (parts.first << 28);
            auto const low = (parts.second.first.second != 0)
                ?  ((parts.second.first.first * parts.second.first.second)
                 + parts.second.second)
                : parts.second.first.first;
            auto const got = (high | low);

            std::cout << std::hex << std::setw(16) << std::setfill('0') << wanted << "\n";
            std::cout << std::hex << std::setw(16) << std::setfill('0') << got << "\n";
            if (wanted != got) {
                std::cerr << "BAD BAD BAD!\n";
                break;
            }
        }
    }

    if constexpr (false) {
        std::cout << viua::arch::ops::to_string(0x0000) << "\n";
        std::cout << viua::arch::ops::to_string(0x0001) << "\n";
        std::cout << viua::arch::ops::to_string(0x1001) << "\n";
        std::cout << viua::arch::ops::to_string(0x9001) << "\n";
        std::cout << viua::arch::ops::to_string(0x1002) << "\n";
        std::cout << viua::arch::ops::to_string(0x1003) << "\n";
        std::cout << viua::arch::ops::to_string(0x1004) << "\n";
        std::cout << viua::arch::ops::to_string(0x5001) << "\n";
    }

    {
        /*
         * If invoked directly, emit a sample executable binary. This makes
         * testing easy as we always can have a sample, working, known-good
         * binary produced.
         */
        if (argc == 1) {
            std::array<viua::arch::instruction_type, 32> text {};
            auto ip = text.data();

            {
                ip = op_li(ip, 0xdeadbeefdeadbeef);
                *ip++ = codec::formats::S{
                    (viua::arch::ops::GREEDY |
                      static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::DELETE))
                    , codec::formats::make_local_access(2)
                }.encode();
                *ip++ = codec::formats::S{
                      static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::DELETE)
                    , codec::formats::make_local_access(3)
                }.encode();
                *ip++ = static_cast<uint64_t>(viua::arch::ops::OPCODE::EBREAK);

                ip = op_li(ip, 42l);
                *ip++ = static_cast<uint64_t>(viua::arch::ops::OPCODE::EBREAK);

                ip = op_li(ip, -1l);
                *ip++ = codec::formats::S{
                    (viua::arch::ops::GREEDY |
                      static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::DELETE))
                    , codec::formats::make_local_access(2)
                }.encode();
                *ip++ = codec::formats::S{
                      static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::DELETE)
                    , codec::formats::make_local_access(3)
                }.encode();
                *ip++ = static_cast<uint64_t>(viua::arch::ops::OPCODE::EBREAK);
                *ip++ = static_cast<uint64_t>(viua::arch::ops::OPCODE::HALT);
            }

            auto const a_out = open(
                  "./a.out"
                , O_CREAT|O_TRUNC|O_WRONLY
                , S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH
            );
            if (a_out == -1) {
                close(a_out);
                exit(1);
            }

            constexpr auto VIUA_MAGIC[[maybe_unused]] = "\x7fVIUA\x00\x00\x00";
            auto const VIUAVM_INTERP = std::string{"viua-vm"};

            {
                auto const ops_count = (ip - text.begin());
                auto const text_size = (ops_count * sizeof(decltype(text)::value_type));
                auto const text_offset = (
                      sizeof(Elf64_Ehdr)
                    + (3 * sizeof(Elf64_Phdr))
                    + (VIUAVM_INTERP.size() + 1));

                // see elf(5)
                Elf64_Ehdr elf_header {};
                elf_header.e_ident[EI_MAG0] = '\x7f';
                elf_header.e_ident[EI_MAG1] = 'E';
                elf_header.e_ident[EI_MAG2] = 'L';
                elf_header.e_ident[EI_MAG3] = 'F';
                elf_header.e_ident[EI_CLASS] = ELFCLASS64;
                elf_header.e_ident[EI_DATA] = ELFDATA2LSB;
                elf_header.e_ident[EI_VERSION] = EV_CURRENT;
                elf_header.e_ident[EI_OSABI] = ELFOSABI_STANDALONE;
                elf_header.e_ident[EI_ABIVERSION] = 0;
                elf_header.e_type = ET_EXEC;
                elf_header.e_machine = ET_NONE;
                elf_header.e_version = elf_header.e_ident[EI_VERSION];
                elf_header.e_entry = text_offset;
                elf_header.e_phoff = sizeof(elf_header);
                elf_header.e_phentsize = sizeof(Elf64_Phdr);
                elf_header.e_phnum = 3;
                elf_header.e_shoff = 0; // FIXME section header table
                elf_header.e_flags = 0; // processor-specific flags, should be 0
                elf_header.e_ehsize = sizeof(elf_header);
                write(a_out, &elf_header, sizeof(elf_header));

                Elf64_Phdr magic_for_binfmt_misc {};
                magic_for_binfmt_misc.p_type = PT_NULL;
                magic_for_binfmt_misc.p_offset = 0;
                memcpy(&magic_for_binfmt_misc.p_offset, VIUA_MAGIC, 8);
                write(a_out, &magic_for_binfmt_misc, sizeof(magic_for_binfmt_misc));

                Elf64_Phdr interpreter {};
                interpreter.p_type = PT_INTERP;
                interpreter.p_offset = (sizeof(elf_header) + 3 * sizeof(Elf64_Phdr));
                interpreter.p_filesz = VIUAVM_INTERP.size() + 1;
                interpreter.p_flags = PF_R;
                write(a_out, &interpreter, sizeof(interpreter));

                Elf64_Phdr text_segment {};
                text_segment.p_type = PT_LOAD;
                text_segment.p_offset = text_offset;
                text_segment.p_filesz = text_size;
                text_segment.p_memsz = text_size;
                text_segment.p_flags = PF_R|PF_X;
                text_segment.p_align = sizeof(viua::arch::instruction_type);
                write(a_out, &text_segment, sizeof(text_segment));

                write(a_out, VIUAVM_INTERP.c_str(), VIUAVM_INTERP.size() + 1);

                write(a_out, text.data(), text_size);
            }

            close(a_out);

            return 0;
        }

        /*
         * If invoked with some operands, use the first of them as the
         * binary to load and execute. It most probably will be the sample
         * executable generated by an earlier invokation of the codec
         * testing program.
         */
        auto const executable_path = std::string{(argc > 1)
            ? argv[1]
            : "./a.out"};
        std::array<viua::arch::instruction_type, 128> text {};
        {
            auto const a_out = open(executable_path.c_str(), O_RDONLY);
            if (a_out == -1) {
                close(a_out);
                exit(1);
            }

            Elf64_Ehdr elf_header {};
            read(a_out, &elf_header, sizeof(elf_header));

            /*
             * We need to skip a few program headers which are just used to make
             * the file a proper ELF as recognised by file(1) and readelf(1).
             */
            Elf64_Phdr program_header {};
            read(a_out, &program_header, sizeof(program_header)); // skip magic PT_NULL
            read(a_out, &program_header, sizeof(program_header)); // skip PT_INTERP

            /*
             * Then comes the actually useful program header describing PT_LOAD
             * segment with .text section containing the instructions we need to
             * run the program.
             */
            read(a_out, &program_header, sizeof(program_header));

            lseek(a_out, program_header.p_offset, SEEK_SET);
            read(a_out, text.data(), program_header.p_filesz);

            std::cout
                << "[vm] loaded " << program_header.p_filesz
                << " byte(s) of .text section from PT_LOAD segment of "
                << executable_path << "\n";
            std::cout
                << "[vm] loaded " << (program_header.p_filesz / sizeof(decltype(text)::value_type))
                << " instructions\n";

            close(a_out);
        }

        auto registers = std::vector<Value>(256);
        run(registers, text.data(), { (executable_path + "[.text]"), text.begin(), text.end() });

        std::cerr << "\n------ 8< ------\n\n";
    }

    return 0;
}
