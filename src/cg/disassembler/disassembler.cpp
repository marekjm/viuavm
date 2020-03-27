/*
 *  Copyright (C) 2015-2019 Marek Marecki
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

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <viua/bytecode/maps.h>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/operand_types.h>
#include <viua/bytecode/codec.h>
#include <viua/cg/disassembler/disassembler.h>
#include <viua/support/env.h>
#include <viua/support/pointer.h>
#include <viua/support/string.h>
#include <viua/util/memory.h>

using viua::util::memory::load_aligned;

namespace disassembler {
static auto intop_with_rs_type(Decoder_type const& decoder, uint8_t const* ptr)
    -> std::tuple<std::string, uint8_t const*>
{
    auto oss = std::ostringstream{};

    auto const type = *reinterpret_cast<OperandType const*>(ptr);

    if (type == OT_VOID) {
        oss << "void";
        ++ptr;
    } else if (type == OT_INT) {
        auto const [ next_ptr, value ] = decoder.decode_i32(ptr);
        oss << value;
        ptr = next_ptr;
    } else if (type == OT_FLOAT) {
        auto const [ next_ptr, value ] = decoder.decode_f64(ptr);
        oss << value;
        ptr = next_ptr;
    } else if (std::set{ OT_REGISTER_INDEX, OT_REGISTER_REFERENCE, OT_POINTER }.count(type)) {
        auto const [ next_ptr, value ] = decoder.decode_register(ptr);

        using viua::bytecode::codec::Access_specifier;
        switch (std::get<2>(value)) {
            case Access_specifier::Direct:
                oss << "%";
                break;
            case Access_specifier::Register_indirect:
                oss << "@";
                break;
            case Access_specifier::Pointer_dereference:
                oss << "*";
                break;
            default:
                throw "invalid access specifier encoded";
        }

        oss << std::get<1>(value) << " ";

        using viua::bytecode::codec::Register_set;
        switch (std::get<0>(value)) {
            case Register_set::Global:
                oss << "global";
                break;
            case Register_set::Local:
                oss << "local";
                break;
            case Register_set::Static:
                oss << "static";
                break;
            case Register_set::Arguments:
                oss << "arguments";
                break;
            case Register_set::Parameters:
                oss << "parameters";
                break;
            case Register_set::Closure_local:
                oss << "";
                break;
            default:
                throw "invalid register set encoded";
        }

        ptr = next_ptr;
    } else if (type == OT_TRUE) {
        oss << "true";
        ++ptr;
    } else if (type == OT_FALSE) {
        oss << "false";
        ++ptr;
    } else {
        throw "invalid operand type encoded";
    }

    return { oss.str(), ptr };
}

static auto disassemble_string(
        std::ostream& oss,
        Decoder_type const& decoder,
        uint8_t const* ptr,
        std::optional<char const> quote = std::nullopt) -> uint8_t const*
{
    ++ptr;  // for operand type
    auto const [ next_ptr, s ] = decoder.decode_string(ptr);
    oss << " " << (quote.has_value() ? str::enquote(s, *quote) : s);
    return next_ptr;
}

static auto disassemble_raw_string(
        std::ostream& oss,
        Decoder_type const& decoder,
        uint8_t const* ptr,
        std::optional<char const> quote = std::nullopt) -> uint8_t const*
{
    auto const [ next_ptr, s ] = decoder.decode_string(ptr);
    oss << " " << (quote.has_value() ? str::enquote(s, *quote) : s);
    return next_ptr;
}

static auto stringify_bit_string_to_hex(std::vector<uint8_t> bytes) -> std::string
{
    if (bytes.empty()) {
        return "0x0";
    }

    static auto const decodings = std::map<uint8_t, char>{
        {
            0b0000,
            '0',
        },
        {
            0b0001,
            '1',
        },
        {
            0b0010,
            '2',
        },
        {
            0b0011,
            '3',
        },
        {
            0b0100,
            '4',
        },
        {
            0b0101,
            '5',
        },
        {
            0b0110,
            '6',
        },
        {
            0b0111,
            '7',
        },
        {
            0b1000,
            '8',
        },
        {
            0b1001,
            '9',
        },
        {
            0b1010,
            'a',
        },
        {
            0b1011,
            'b',
        },
        {
            0b1100,
            'c',
        },
        {
            0b1101,
            'd',
        },
        {
            0b1110,
            'e',
        },
        {
            0b1111,
            'f',
        },
    };
    constexpr auto mask_high = uint8_t{0b00001111};
    constexpr auto mask_low  = uint8_t{0b11110000};

    auto oss = std::ostringstream{};
    oss << "0x";

    std::reverse(bytes.begin(), bytes.end());
    for (auto const each : bytes) {
        auto high_digit = ((each & mask_low) >> 4);
        auto low_digit  = (each & mask_high);

        oss << decodings.at(static_cast<uint8_t>(high_digit));
        oss << decodings.at(static_cast<uint8_t>(low_digit));
    }

    return oss.str();
}
static auto stringify_bit_string_to_binary(std::vector<uint8_t> bytes) -> std::string
{
    if (bytes.empty()) {
        return "0b0";
    }

    auto oss = std::ostringstream{};
    oss << "0b";

    std::reverse(bytes.begin(), bytes.end());
    for (auto const each : bytes) {
        constexpr auto one = uint8_t{1};

        for (auto i = size_t{0}; i < 8; ++i) {
            oss << static_cast<bool>(each & (one << (7 - i)));
        }
    }

    return oss.str();
}
static auto stringify_bit_string(std::vector<uint8_t> bytes) -> std::string
{
    if (bytes.size() <= 2) {
        return stringify_bit_string_to_binary(std::move(bytes));
    } else {
        return stringify_bit_string_to_hex(std::move(bytes));
    }
}
static auto disassemble_bit_string(std::ostream& oss,
                                   Decoder_type const& decoder,
                                   uint8_t const* ptr)
    -> uint8_t const*
{
    auto const [ next_ptr, bits ] = decoder.decode_bits_string(ptr);
    oss << " " << stringify_bit_string(bits);
    return next_ptr;
}

static auto disassemble_timeout(std::ostream& oss, Decoder_type const& decoder, uint8_t const* ptr)
    -> uint8_t const*
{
    auto const [ next_ptr, value ] = decoder.decode_timeout(ptr);

    oss << " ";
    if (value == 0) {
        oss << "infinity";
    } else {
        oss << (value - 1) << "ms";
    }

    return next_ptr;
}

static auto disassemble_address(std::ostream& oss, Decoder_type const& decoder, uint8_t const* ptr)
    -> uint8_t const*
{
    auto const [ next_ptr, value ] = decoder.decode_address(ptr);

    oss << " 0x";
    oss << std::hex;
    oss << value;
    oss << std::dec;

    return next_ptr;
}

static auto disassemble_ri_operand_with_rs_type(
    std::ostream& oss,
    Decoder_type const& decoder,
    uint8_t const* ptr) -> uint8_t const*
{
    auto const [ s, next_ptr ] = disassembler::intop_with_rs_type(decoder, ptr);
    oss << ' ' << s;
    return next_ptr;
}
auto instruction(Decoder_type const& decoder, uint8_t const* ptr)
    -> std::tuple<std::string, viua::bytecode::codec::bytecode_size_type>
{
    auto saved_ptr = ptr;

    auto const op = OPCODE(*saved_ptr);
    auto opname   = std::string{};
    try {
        opname = OP_NAMES.at(op);
        ++ptr;
    } catch (std::out_of_range const& e) {
        auto emsg = std::ostringstream{};
        emsg << "could not find name for opcode: " << op;
        throw emsg.str();
    }

    auto oss = std::ostringstream{};
    oss << opname;

    if (op == STRING) {
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        ptr = disassemble_string(oss, decoder, ptr, '"');
    } else if (op == TEXT) {
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);

        if (OperandType(*ptr) == OT_REGISTER_INDEX
            or OperandType(*ptr) == OT_POINTER) {
            oss << ' ';
            ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        } else {
            ptr = disassemble_string(oss, decoder, ptr, '"');
        }
    } else if (op == ATOM) {
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        ptr = disassemble_raw_string(oss, decoder, ptr, '\'');
    } else if ((op == CLOSURE) or (op == FUNCTION)) {
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        ptr = disassemble_raw_string(oss, decoder, ptr);
    } else if ((op == CALL) or (op == PROCESS)) {
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);

        if (OperandType(*ptr) == OT_REGISTER_INDEX
            or OperandType(*ptr) == OT_POINTER) {
            oss << ' ';
            ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        } else {
            ptr = disassemble_raw_string(oss, decoder, ptr);
        }
    } else if (op == TAILCALL or op == DEFER) {
        if (OperandType(*ptr) == OT_REGISTER_INDEX
            or OperandType(*ptr) == OT_POINTER) {
            oss << ' ';
            ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        } else {
            ptr = disassemble_raw_string(oss, decoder, ptr);
        }
    } else if ((op == IMPORT) or (op == ENTER) or (op == WATCHDOG)) {
        ptr = disassemble_raw_string(oss, decoder, ptr);
    } else if (op == CATCH) {
        ptr = disassemble_raw_string(oss, decoder, ptr, '"');
        ptr = disassemble_raw_string(oss, decoder, ptr);
    }

    switch (op) {
    case NOP:
    case TRY:
    case LEAVE:
    case RETURN:
    case HALT:
        // These ops have no operands.
        break;
    case STRING:
    case TEXT:
    case ATOM:
    case CLOSURE:
    case FUNCTION:
    case CALL:
    case PROCESS:
    case TAILCALL:
    case DEFER:
    case IMPORT:
    case ENTER:
    case WATCHDOG:
    case CATCH:
        // Already handled in the `if` above.
        break;
    case BITSWIDTH:
    case BITSEQ:
    case BITSLT:
    case BITSLTE:
    case BITSGT:
    case BITSGTE:
    case BITAEQ:
    case BITALT:
    case BITALTE:
    case BITAGT:
    case BITAGTE:
        // Not implemented.
        break;
    case IZERO:
    case PRINT:
    case ECHO:
    case THROW:
    case DRAW:
    case DELETE:
    case IINC:
    case IDEC:
    case SELF:
    case ALLOCATE_REGISTERS:
    case STRUCT:
    case WRAPINCREMENT:
    case WRAPDECREMENT:
    case CHECKEDSINCREMENT:
    case CHECKEDSDECREMENT:
    case CHECKEDUINCREMENT:
    case CHECKEDUDECREMENT:
    case SATURATINGSINCREMENT:
    case SATURATINGSDECREMENT:
    case SATURATINGUINCREMENT:
    case SATURATINGUDECREMENT:
    case BOOL:
    case FRAME:
    case IO_CANCEL:
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        break;
    case INTEGER:
    case SEND:
    case ITOF:
    case FTOI:
    case STOI:
    case STOF:
    case ISNULL:
    case NOT:
    case MOVE:
    case COPY:
    case PTR:
    case PTRLIVE:
    case SWAP:
    case VPUSH:
    case VLEN:
    case TEXTLENGTH:
    case STRUCTKEYS:
    case BITNOT:
    case ROL:
    case ROR:
    case BITS_OF_INTEGER:
    case INTEGER_OF_BITS:
    case IO_CLOSE:
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        break;
    case BITS:
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);

        if (OperandType(*ptr) == OT_BITS) {
            ptr = disassemble_bit_string(oss, decoder, ptr);
        } else {
            ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        }

        break;
    case ADD:
    case SUB:
    case MUL:
    case DIV:
    case LT:
    case LTE:
    case GT:
    case GTE:
    case EQ:
    case BITAND:
    case BITOR:
    case BITXOR:
    case BITAT:
    case SHL:
    case SHR:
    case ASHL:
    case ASHR:
    case WRAPADD:
    case WRAPSUB:
    case WRAPMUL:
    case WRAPDIV:
    case CHECKEDSADD:
    case CHECKEDSSUB:
    case CHECKEDSMUL:
    case CHECKEDSDIV:
    case CHECKEDUADD:
    case CHECKEDUSUB:
    case CHECKEDUMUL:
    case CHECKEDUDIV:
    case SATURATINGSADD:
    case SATURATINGSSUB:
    case SATURATINGSMUL:
    case SATURATINGSDIV:
    case SATURATINGUADD:
    case SATURATINGUSUB:
    case SATURATINGUMUL:
    case SATURATINGUDIV:
    case BITSET:
    case VECTOR:
    case VAT:
    case CAPTURE:
    case CAPTURECOPY:
    case CAPTUREMOVE:
    case AND:
    case OR:
    case TEXTEQ:
    case TEXTAT:
    case TEXTCOMMONPREFIX:
    case TEXTCOMMONSUFFIX:
    case TEXTCONCAT:
    case VINSERT:
    case VPOP:
    case ATOMEQ:
    case PIDEQ:
    case STRUCTINSERT:
    case STRUCTREMOVE:
    case STRUCTAT:
    case STREQ:
    case IO_READ:
    case IO_WRITE:
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        break;
    case TEXTSUB:
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        break;
    case JUMP:
        ptr = disassemble_address(oss, decoder, ptr);
        break;
    case IF:
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        ptr = disassemble_address(oss, decoder, ptr);
        ptr = disassemble_address(oss, decoder, ptr);
        break;
    case FLOAT:
    {
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        auto const [ next_ptr, value ] = decoder.decode_f64(ptr);
        oss << " " << value;
        ptr = next_ptr;
        break;
    }
    case JOIN:
    case IO_WAIT:
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        ptr = disassemble_timeout(oss, decoder, ptr);
        break;
    case RECEIVE:
        ptr = disassemble_ri_operand_with_rs_type(oss, decoder, ptr);
        ptr = disassemble_timeout(oss, decoder, ptr);
        break;
    default:
        // if opcode was not covered here, it means it must have been a
        // variable-length opcode
        oss << "";
    }

    if (ptr < saved_ptr) {
        throw("bytecode pointer increase less than zero: near "
              + OP_NAMES.at(op) + " instruction");
    }
    // we already *know* that the result will not be negative
    auto const increase =
        static_cast<viua::bytecode::codec::bytecode_size_type>(ptr - saved_ptr);

    return std::tuple<std::string, viua::bytecode::codec::bytecode_size_type>(
        oss.str(), increase);
}
}
