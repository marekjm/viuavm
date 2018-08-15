/*
 *  Copyright (C) 2016, 2017, 2018 Marek Marecki
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
#include <math.h>
#include <tuple>
#include <viua/bytecode/bytetypedef.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/tools.h>
#include <viua/support/string.h>
using namespace std;


using viua::cg::lex::Token;
using TokenVector        = std::vector<Token>;
using bytecode_size_type = viua::internals::types::bytecode_size;


namespace viua { namespace cg { namespace tools {
static auto size_of_register_index_operand_with_rs_type(
    TokenVector const& tokens,
    TokenVector::size_type i) -> std::tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{0};

    if (tokens.at(i) == "void" or tokens.at(i) == "true"
        or tokens.at(i) == "false") {
        calculated_size += sizeof(viua::internals::types::byte);
        ++i;
    } else if (auto const sigil = tokens.at(i).str().at(0); (sigil == '%' or sigil == '@' or sigil == '*')
               and str::isnum(tokens.at(i).str().substr(1))) {
        calculated_size += sizeof(viua::internals::types::byte);
        calculated_size += sizeof(viua::internals::Register_sets);
        calculated_size += sizeof(viua::internals::types::register_index);
        ++i;

        ++i;  // for register set specifier
    } else {
        throw viua::cg::lex::Invalid_syntax(
            tokens.at(i), ("invalid operand token: " + tokens.at(i).str()));
    }

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_register_index_operand(TokenVector const& tokens,
                                           TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{0};

    if (tokens.at(i) == "static" or tokens.at(i) == "local"
        or tokens.at(i) == "global") {
        ++i;
    }

    if (tokens.at(i) == "void" or tokens.at(i) == "true"
        or tokens.at(i) == "false") {
        calculated_size += sizeof(viua::internals::types::byte);
        ++i;
    } else if (auto const sigil = tokens.at(i).str().at(0); (sigil == '%' or sigil == '@' or sigil == '*')
               and str::isnum(tokens.at(i).str().substr(1))) {
        calculated_size += sizeof(viua::internals::types::byte);
        calculated_size += sizeof(viua::internals::Register_sets);
        calculated_size += sizeof(viua::internals::types::register_index);
        ++i;
    } else {
        throw viua::cg::lex::Invalid_syntax(
            tokens.at(i), ("invalid operand token: " + tokens.at(i).str()));
    }

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}

static auto size_of_instruction_with_no_operands(TokenVector const&,
                                                 TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto const calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};
    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_instruction_with_one_ri_operand_with_rs_type(
    TokenVector const& tokens,
    TokenVector::size_type i) -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_instruction_with_two_ri_operands_with_rs_types(
    TokenVector const& tokens,
    TokenVector::size_type i) -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for source register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_instruction_with_two_ri_operands(TokenVector const& tokens,
                                                     TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) = size_of_register_index_operand(tokens, i);
    calculated_size += size_increment;

    // for source register
    tie(size_increment, i) = size_of_register_index_operand(tokens, i);
    calculated_size += size_increment;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_instruction_with_three_ri_operands_with_rs_types(
    TokenVector const& tokens,
    TokenVector::size_type i) -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for 1st source register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for 2nd source register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_instruction_with_four_ri_operands_with_rs_types(
    TokenVector const& tokens,
    TokenVector::size_type i) -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for 1st source register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for 2nd source register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for 3rd source register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_instruction_with_three_ri_operands(
    TokenVector const& tokens,
    TokenVector::size_type i) -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) = size_of_register_index_operand(tokens, i);
    calculated_size += size_increment;

    // for 1st source register
    tie(size_increment, i) = size_of_register_index_operand(tokens, i);
    calculated_size += size_increment;

    // for 2nd source register
    tie(size_increment, i) = size_of_register_index_operand(tokens, i);
    calculated_size += size_increment;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_instruction_alu(TokenVector const& tokens,
                                    TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
}

static auto size_of_binary_literal_operand(TokenVector const& tokens,
                                           TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};
    calculated_size += sizeof(viua::internals::types::bits_size);

    auto const literal = tokens.at(i).str().substr(2);
    ++i;

    /*
     * Size is first multiplied by 1, because every character of the
     * string may be encoded on one bit.
     * Then the result is divided by 8.0 because a byte has 8 bits, and
     * the bytecode is encoded using bytes.
     */
    auto const size =
        (assembler::operands::normalise_binary_literal(literal).size() / 8);
    calculated_size += size;

    return tuple<bytecode_size_type, decltype(i)>{calculated_size, i};
}

static auto size_of_octal_literal_operand(TokenVector const& tokens,
                                          TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};
    calculated_size += sizeof(viua::internals::types::bits_size);

    auto literal = tokens.at(i).str();
    ++i;

    calculated_size +=
        (assembler::operands::normalise_binary_literal(
             assembler::operands::octal_to_binary_literal(literal))
             .size()
         / 8);

    return tuple<bytecode_size_type, decltype(i)>{calculated_size, i};
}

static auto size_of_hexadecimal_literal_operand(TokenVector const& tokens,
                                                TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};
    calculated_size += sizeof(viua::internals::types::bits_size);

    auto literal = tokens.at(i).str();
    ++i;

    calculated_size +=
        (assembler::operands::normalise_binary_literal(
             assembler::operands::hexadecimal_to_binary_literal(literal))
             .size()
         / 8);

    return tuple<bytecode_size_type, decltype(i)>{calculated_size, i};
}

static auto const size_of_nop   = size_of_instruction_with_no_operands;
static auto const size_of_izero = size_of_instruction_with_one_ri_operand_with_rs_type;
static auto size_of_istore(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{0};
    std::tie(calculated_size, i) =
        size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);

    calculated_size += sizeof(viua::internals::types::byte);
    calculated_size += sizeof(viua::internals::types::plain_int);
    ++i;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto const size_of_iinc = size_of_instruction_with_one_ri_operand_with_rs_type;
static auto const size_of_idec = size_of_instruction_with_one_ri_operand_with_rs_type;
static auto size_of_fstore(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) = size_of_register_index_operand(tokens, i);
    calculated_size += size_increment;

    calculated_size += sizeof(viua::internals::types::plain_float);
    ++i;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto const size_of_itof =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_ftoi =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_stoi =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_stof =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_add = size_of_instruction_alu;
static auto const size_of_sub = size_of_instruction_alu;
static auto const size_of_mul = size_of_instruction_alu;
static auto const size_of_div = size_of_instruction_alu;
static auto const size_of_lt  = size_of_instruction_alu;
static auto const size_of_lte = size_of_instruction_alu;
static auto const size_of_gt  = size_of_instruction_alu;
static auto const size_of_gte = size_of_instruction_alu;
static auto const size_of_eq  = size_of_instruction_alu;
static auto size_of_string(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    ++calculated_size;                                       // for operand type
    calculated_size += tokens.at(i++).str().size() + 1 - 2;  // +1 for null
                                                             // terminator, -2
                                                             // for quotes

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto const size_of_streq = size_of_instruction_with_three_ri_operands;
static auto size_of_text(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    if (tokens.at(i).str().at(0) == '*' or tokens.at(i).str().at(0) == '%') {
        tie(size_increment, i) =
            size_of_register_index_operand_with_rs_type(tokens, i);
        calculated_size += size_increment;
    } else {
        ++calculated_size;  // for operand type
        calculated_size +=
            tokens.at(i++).str().size() + 1 - 2;  // +1 for null terminator, -2
                                                  // for quotes
    }

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto const size_of_texteq =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_textat =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_textsub =
    size_of_instruction_with_four_ri_operands_with_rs_types;
static auto const size_of_textlength =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_textcommonprefix =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_textcommonsuffix =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_textconcat =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto size_of_vec(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for pack start register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for pack count register
    tie(size_increment, i) = size_of_register_index_operand(tokens, i);
    calculated_size += size_increment;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_vinsert(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for source register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for position index
    tie(size_increment, i) = size_of_register_index_operand(tokens, i);
    calculated_size += size_increment;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto const size_of_vpush =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_vpop =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_vat =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_vlen =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_bool = size_of_instruction_with_one_ri_operand_with_rs_type;
static auto const size_of_not =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_ptrlive =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_and =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_or =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto size_of_bits(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    if (tokens.at(i).str().at(0) == '*' or tokens.at(i).str().at(0) == '%') {
        tie(size_increment, i) =
            size_of_register_index_operand_with_rs_type(tokens, i);
        calculated_size += size_increment;
    } else if (tokens.at(i).str().at(1) == 'b') {
        tie(size_increment, i) = size_of_binary_literal_operand(tokens, i);
        calculated_size += size_increment;
    } else if (tokens.at(i).str().at(1) == 'o') {
        tie(size_increment, i) = size_of_octal_literal_operand(tokens, i);
        calculated_size += size_increment;
    } else if (tokens.at(i).str().at(1) == 'x') {
        tie(size_increment, i) = size_of_hexadecimal_literal_operand(tokens, i);
        calculated_size += size_increment;
    }

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto const size_of_bitand =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_bitor =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_bitnot =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_bitxor =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_bitat =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_bitset =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_shl =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_shr =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_ashl =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_ashr =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_rol =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_ror =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_wrapincrement =
    size_of_instruction_with_one_ri_operand_with_rs_type;
static auto const size_of_wrapdecrement =
    size_of_instruction_with_one_ri_operand_with_rs_type;
static auto const size_of_wrapadd =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_wrapsub =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_wrapmul =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_wrapdiv =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_checkedsincrement =
    size_of_instruction_with_one_ri_operand_with_rs_type;
static auto const size_of_checkedsdecrement =
    size_of_instruction_with_one_ri_operand_with_rs_type;
static auto const size_of_checkedsadd =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_checkedssub =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_checkedsmul =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_checkedsdiv =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_checkeduincrement =
    size_of_instruction_with_one_ri_operand_with_rs_type;
static auto const size_of_checkedudecrement =
    size_of_instruction_with_one_ri_operand_with_rs_type;
static auto const size_of_checkeduadd =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_checkedusub =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_checkedumul =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_checkedudiv =
    size_of_instruction_with_three_ri_operands_with_rs_types;

static auto const size_of_saturatingsincrement =
    size_of_instruction_with_one_ri_operand_with_rs_type;
static auto const size_of_saturatingsdecrement =
    size_of_instruction_with_one_ri_operand_with_rs_type;
static auto const size_of_saturatingsadd =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_saturatingssub =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_saturatingsmul =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_saturatingsdiv =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_saturatinguincrement =
    size_of_instruction_with_one_ri_operand_with_rs_type;
static auto const size_of_saturatingudecrement =
    size_of_instruction_with_one_ri_operand_with_rs_type;
static auto const size_of_saturatinguadd =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_saturatingusub =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_saturatingumul =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto const size_of_saturatingudiv =
    size_of_instruction_with_three_ri_operands_with_rs_types;

static auto const size_of_move =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_copy =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_ptr =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_swap =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto const size_of_delete = size_of_instruction_with_one_ri_operand_with_rs_type;
static auto const size_of_isnull =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto size_of_print = size_of_instruction_with_one_ri_operand_with_rs_type;
static auto size_of_echo  = size_of_instruction_with_one_ri_operand_with_rs_type;
static auto size_of_capture(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for inside-closure register
    tie(size_increment, i) = size_of_register_index_operand(tokens, i);
    calculated_size += size_increment;

    // for source register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_capturecopy(TokenVector const& tokens,
                                TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for inside-closure register
    tie(size_increment, i) = size_of_register_index_operand(tokens, i);
    calculated_size += size_increment;

    // for source register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_capturemove(TokenVector const& tokens,
                                TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for inside-closure register
    tie(size_increment, i) = size_of_register_index_operand(tokens, i);
    calculated_size += size_increment;

    // for source register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_closure(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    calculated_size += tokens.at(i).str().size() + 1;
    ++i;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_function(TokenVector const& tokens,
                             TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    calculated_size += tokens.at(i).str().size() + 1;
    ++i;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_frame = size_of_instruction_with_two_ri_operands;
static auto size_of_call(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    if (tokens.at(i).str().at(0) == '*' or tokens.at(i).str().at(0) == '%') {
        tie(size_increment, i) =
            size_of_register_index_operand_with_rs_type(tokens, i);
        calculated_size += size_increment;
    } else {
        calculated_size += tokens.at(i).str().size() + 1;
        ++i;
    }

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_tailcall(TokenVector const& tokens,
                             TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    if (tokens.at(i).str().at(0) == '*' or tokens.at(i).str().at(0) == '%') {
        auto size_increment = decltype(calculated_size){0};
        tie(size_increment, i) =
            size_of_register_index_operand_with_rs_type(tokens, i);
        calculated_size += size_increment;
    } else {
        calculated_size += tokens.at(i).str().size() + 1;
        ++i;
    }

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_defer = size_of_tailcall;
static auto size_of_arg(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    // for source register
    tie(size_increment, i) = size_of_register_index_operand(tokens, i);
    calculated_size += size_increment;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_process(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    calculated_size += tokens.at(i).str().size() + 1;
    ++i;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_self = size_of_instruction_with_one_ri_operand_with_rs_type;
static auto size_of_join(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{0};
    tie(calculated_size, i) =
        size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);

    if (str::is_timeout_literal(tokens.at(i))) {
        calculated_size += sizeof(viua::internals::types::byte);
        calculated_size += sizeof(viua::internals::types::timeout);
        ++i;
    } else {
        throw viua::cg::lex::Invalid_syntax(tokens.at(i),
                                            "invalid timeout token in 'join'");
    }

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_send =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto size_of_receive(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{0};
    tie(calculated_size, i) =
        size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);

    if (str::is_timeout_literal(tokens.at(i))) {
        calculated_size += sizeof(viua::internals::types::byte);
        calculated_size += sizeof(viua::internals::types::timeout);
        ++i;
    } else {
        throw viua::cg::lex::Invalid_syntax(
            tokens.at(i), "invalid timeout token in 'receive'");
    }

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_watchdog(TokenVector const& tokens,
                             TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode

    calculated_size += tokens.at(i).str().size() + 1;
    ++i;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_jump(TokenVector const&, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};
    calculated_size += sizeof(bytecode_size_type);
    ++i;
    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_if(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};

    auto size_increment = decltype(calculated_size){0};
    // for source register
    tie(size_increment, i) = size_of_register_index_operand(tokens, i);
    calculated_size += size_increment;

    calculated_size += 2 * sizeof(bytecode_size_type);
    i += 2;

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_throw =
    size_of_instruction_with_one_ri_operand_with_rs_type;
static auto size_of_catch(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};

    calculated_size += tokens.at(i++).str().size() + 1 - 2;  // +1 for null
                                                             // terminator, -2
                                                             // for quotes
    calculated_size += tokens.at(i++).str().size() + 1;      // +1 for null
                                                             // terminator

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_pull = size_of_instruction_with_one_ri_operand_with_rs_type;
static auto size_of_try(TokenVector const&, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};
    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_enter(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};

    calculated_size += tokens.at(i++).str().size() + 1;  // +1 for null
                                                         // terminator

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_leave(TokenVector const&, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};
    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_import(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};

    calculated_size += tokens.at(i++).str().size() + 1 - 2;  // +1 for null
                                                             // terminator, -2
                                                             // for quotes

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}

static auto size_of_atom(TokenVector const& tokens, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};

    auto size_increment = decltype(calculated_size){0};

    // for target register
    tie(size_increment, i) =
        size_of_register_index_operand_with_rs_type(tokens, i);
    calculated_size += size_increment;

    calculated_size += tokens.at(i++).str().size() + 1 - 2;  // +1 for null
                                                             // terminator, -2
                                                             // for quotes

    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_atomeq =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto size_of_struct = size_of_instruction_with_one_ri_operand_with_rs_type;
static auto size_of_structinsert =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto size_of_structremove =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto size_of_structkeys =
    size_of_instruction_with_two_ri_operands_with_rs_types;
static auto size_of_insert =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto size_of_remove =
    size_of_instruction_with_three_ri_operands_with_rs_types;
static auto size_of_return(TokenVector const&, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size = bytecode_size_type{
        sizeof(viua::internals::types::byte)};  // start with the size of a
                                                // single opcode
    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}
static auto size_of_halt(TokenVector const&, TokenVector::size_type i)
    -> tuple<bytecode_size_type, decltype(i)> {
    auto calculated_size =
        bytecode_size_type{sizeof(viua::internals::types::byte)};
    return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
}

auto calculate_bytecode_size_of_first_n_instructions2(
    TokenVector const& tokens,
    const std::remove_reference<decltype(tokens)>::type::size_type
        instructions_counter) -> bytecode_size_type {
    auto bytes = bytecode_size_type{0};

    auto const limit = tokens.size();
    auto counted_instructions =
        std::remove_const_t<decltype(instructions_counter)>{0};
    for (auto i = TokenVector::size_type{0};
         (i < limit) and (counted_instructions < instructions_counter);
         ++i) {
        auto const& token = tokens.at(i);

        if (token == ".function:" or token == ".closure:" or token == ".block:"
            or token == ".mark:" or token == ".import:"
            or token == ".signature:" or token == ".bsignature:"
            or token == ".unused:") {
            ++i;

            if (tokens.at(i) == "[[") {
                do {
                    ++i;
                } while (tokens.at(i) != "]]");
                ++i;
            }

            continue;
        }
        if (token == ".info:") {
            i += 2;
            continue;
        }
        if (token == ".name:") {
            i += 2;
            continue;
        }
        if (token == ".end") {
            continue;
        }
        if (token == "\n") {
            continue;
        }

        auto increase = bytecode_size_type{0};
        if (tokens.at(i) == "nop") {
            ++i;
            tie(increase, i) = size_of_nop(tokens, i);
        } else if (tokens.at(i) == "izero") {
            ++i;
            tie(increase, i) = size_of_izero(tokens, i);
        } else if (tokens.at(i) == "integer") {
            ++i;
            tie(increase, i) = size_of_istore(tokens, i);
        } else if (tokens.at(i) == "iinc") {
            ++i;
            tie(increase, i) = size_of_iinc(tokens, i);
        } else if (tokens.at(i) == "idec") {
            ++i;
            tie(increase, i) = size_of_idec(tokens, i);
        } else if (tokens.at(i) == "float") {
            ++i;
            tie(increase, i) = size_of_fstore(tokens, i);
        } else if (tokens.at(i) == "itof") {
            ++i;
            tie(increase, i) = size_of_itof(tokens, i);
        } else if (tokens.at(i) == "ftoi") {
            ++i;
            tie(increase, i) = size_of_ftoi(tokens, i);
        } else if (tokens.at(i) == "stoi") {
            ++i;
            tie(increase, i) = size_of_stoi(tokens, i);
        } else if (tokens.at(i) == "stof") {
            ++i;
            tie(increase, i) = size_of_stof(tokens, i);
        } else if (tokens.at(i) == "add") {
            ++i;
            tie(increase, i) = size_of_add(tokens, i);
        } else if (tokens.at(i) == "sub") {
            ++i;
            tie(increase, i) = size_of_sub(tokens, i);
        } else if (tokens.at(i) == "mul") {
            ++i;
            tie(increase, i) = size_of_mul(tokens, i);
        } else if (tokens.at(i) == "div") {
            ++i;
            tie(increase, i) = size_of_div(tokens, i);
        } else if (tokens.at(i) == "lt") {
            ++i;
            tie(increase, i) = size_of_lt(tokens, i);
        } else if (tokens.at(i) == "lte") {
            ++i;
            tie(increase, i) = size_of_lte(tokens, i);
        } else if (tokens.at(i) == "gt") {
            ++i;
            tie(increase, i) = size_of_gt(tokens, i);
        } else if (tokens.at(i) == "gte") {
            ++i;
            tie(increase, i) = size_of_gte(tokens, i);
        } else if (tokens.at(i) == "eq") {
            ++i;
            tie(increase, i) = size_of_eq(tokens, i);
        } else if (tokens.at(i) == "string") {
            ++i;
            tie(increase, i) = size_of_string(tokens, i);
        } else if (tokens.at(i) == "text") {
            ++i;
            tie(increase, i) = size_of_text(tokens, i);
        } else if (tokens.at(i) == "texteq") {
            ++i;
            tie(increase, i) = size_of_texteq(tokens, i);
        } else if (tokens.at(i) == "textat") {
            ++i;
            tie(increase, i) = size_of_textat(tokens, i);
        } else if (tokens.at(i) == "textsub") {
            ++i;
            tie(increase, i) = size_of_textsub(tokens, i);
        } else if (tokens.at(i) == "textlength") {
            ++i;
            tie(increase, i) = size_of_textlength(tokens, i);
        } else if (tokens.at(i) == "textcommonprefix") {
            ++i;
            tie(increase, i) = size_of_textcommonprefix(tokens, i);
        } else if (tokens.at(i) == "textcommonsuffix") {
            ++i;
            tie(increase, i) = size_of_textcommonsuffix(tokens, i);
        } else if (tokens.at(i) == "textconcat") {
            ++i;
            tie(increase, i) = size_of_textconcat(tokens, i);
        } else if (tokens.at(i) == "streq") {
            ++i;
            tie(increase, i) = size_of_streq(tokens, i);
        } else if (tokens.at(i) == "vector") {
            ++i;
            tie(increase, i) = size_of_vec(tokens, i);
        } else if (tokens.at(i) == "vinsert") {
            ++i;
            tie(increase, i) = size_of_vinsert(tokens, i);
        } else if (tokens.at(i) == "vpush") {
            ++i;
            tie(increase, i) = size_of_vpush(tokens, i);
        } else if (tokens.at(i) == "vpop") {
            ++i;
            tie(increase, i) = size_of_vpop(tokens, i);
        } else if (tokens.at(i) == "vat") {
            ++i;
            tie(increase, i) = size_of_vat(tokens, i);
        } else if (tokens.at(i) == "vlen") {
            ++i;
            tie(increase, i) = size_of_vlen(tokens, i);
        } else if (tokens.at(i) == "bool") {
            ++i;
            tie(increase, i) = size_of_bool(tokens, i);
        } else if (tokens.at(i) == "not") {
            ++i;
            tie(increase, i) = size_of_not(tokens, i);
        } else if (tokens.at(i) == "and") {
            ++i;
            tie(increase, i) = size_of_and(tokens, i);
        } else if (tokens.at(i) == "or") {
            ++i;
            tie(increase, i) = size_of_or(tokens, i);
        } else if (tokens.at(i) == "bits") {
            ++i;
            tie(increase, i) = size_of_bits(tokens, i);
        } else if (tokens.at(i) == "bitand") {
            ++i;
            tie(increase, i) = size_of_bitand(tokens, i);
        } else if (tokens.at(i) == "bitor") {
            ++i;
            tie(increase, i) = size_of_bitor(tokens, i);
        } else if (tokens.at(i) == "bitnot") {
            ++i;
            tie(increase, i) = size_of_bitnot(tokens, i);
        } else if (tokens.at(i) == "bitxor") {
            ++i;
            tie(increase, i) = size_of_bitxor(tokens, i);
        } else if (tokens.at(i) == "bitat") {
            ++i;
            tie(increase, i) = size_of_bitat(tokens, i);
        } else if (tokens.at(i) == "bitset") {
            ++i;
            tie(increase, i) = size_of_bitset(tokens, i);
        } else if (tokens.at(i) == "shl") {
            ++i;
            tie(increase, i) = size_of_shl(tokens, i);
        } else if (tokens.at(i) == "shr") {
            ++i;
            tie(increase, i) = size_of_shr(tokens, i);
        } else if (tokens.at(i) == "ashl") {
            ++i;
            tie(increase, i) = size_of_ashl(tokens, i);
        } else if (tokens.at(i) == "ashr") {
            ++i;
            tie(increase, i) = size_of_ashr(tokens, i);
        } else if (tokens.at(i) == "rol") {
            ++i;
            tie(increase, i) = size_of_rol(tokens, i);
        } else if (tokens.at(i) == "ror") {
            ++i;
            tie(increase, i) = size_of_ror(tokens, i);
        } else if (tokens.at(i) == "wrapincrement") {
            ++i;
            tie(increase, i) = size_of_wrapincrement(tokens, i);
        } else if (tokens.at(i) == "wrapdecrement") {
            ++i;
            tie(increase, i) = size_of_wrapdecrement(tokens, i);
        } else if (tokens.at(i) == "wrapadd") {
            ++i;
            tie(increase, i) = size_of_wrapadd(tokens, i);
        } else if (tokens.at(i) == "wrapsub") {
            ++i;
            tie(increase, i) = size_of_wrapsub(tokens, i);
        } else if (tokens.at(i) == "wrapmul") {
            ++i;
            tie(increase, i) = size_of_wrapmul(tokens, i);
        } else if (tokens.at(i) == "wrapdiv") {
            ++i;
            tie(increase, i) = size_of_wrapdiv(tokens, i);
        } else if (tokens.at(i) == "checkedsincrement") {
            ++i;
            tie(increase, i) = size_of_checkedsincrement(tokens, i);
        } else if (tokens.at(i) == "checkedsdecrement") {
            ++i;
            tie(increase, i) = size_of_checkedsdecrement(tokens, i);
        } else if (tokens.at(i) == "checkedsadd") {
            ++i;
            tie(increase, i) = size_of_checkedsadd(tokens, i);
        } else if (tokens.at(i) == "checkedssub") {
            ++i;
            tie(increase, i) = size_of_checkedssub(tokens, i);
        } else if (tokens.at(i) == "checkedsmul") {
            ++i;
            tie(increase, i) = size_of_checkedsmul(tokens, i);
        } else if (tokens.at(i) == "checkedsdiv") {
            ++i;
            tie(increase, i) = size_of_checkedsdiv(tokens, i);
        } else if (tokens.at(i) == "checkeduincrement") {
            ++i;
            tie(increase, i) = size_of_checkeduincrement(tokens, i);
        } else if (tokens.at(i) == "checkedudecrement") {
            ++i;
            tie(increase, i) = size_of_checkedudecrement(tokens, i);
        } else if (tokens.at(i) == "checkeduadd") {
            ++i;
            tie(increase, i) = size_of_checkeduadd(tokens, i);
        } else if (tokens.at(i) == "checkedusub") {
            ++i;
            tie(increase, i) = size_of_checkedusub(tokens, i);
        } else if (tokens.at(i) == "checkedumul") {
            ++i;
            tie(increase, i) = size_of_checkedumul(tokens, i);
        } else if (tokens.at(i) == "checkedudiv") {
            ++i;
            tie(increase, i) = size_of_checkedudiv(tokens, i);
        } else if (tokens.at(i) == "saturatingsincrement") {
            ++i;
            tie(increase, i) = size_of_saturatingsincrement(tokens, i);
        } else if (tokens.at(i) == "saturatingsdecrement") {
            ++i;
            tie(increase, i) = size_of_saturatingsdecrement(tokens, i);
        } else if (tokens.at(i) == "saturatingsadd") {
            ++i;
            tie(increase, i) = size_of_saturatingsadd(tokens, i);
        } else if (tokens.at(i) == "saturatingssub") {
            ++i;
            tie(increase, i) = size_of_saturatingssub(tokens, i);
        } else if (tokens.at(i) == "saturatingsmul") {
            ++i;
            tie(increase, i) = size_of_saturatingsmul(tokens, i);
        } else if (tokens.at(i) == "saturatingsdiv") {
            ++i;
            tie(increase, i) = size_of_saturatingsdiv(tokens, i);
        } else if (tokens.at(i) == "saturatinguincrement") {
            ++i;
            tie(increase, i) = size_of_saturatinguincrement(tokens, i);
        } else if (tokens.at(i) == "saturatingudecrement") {
            ++i;
            tie(increase, i) = size_of_saturatingudecrement(tokens, i);
        } else if (tokens.at(i) == "saturatinguadd") {
            ++i;
            tie(increase, i) = size_of_saturatinguadd(tokens, i);
        } else if (tokens.at(i) == "saturatingusub") {
            ++i;
            tie(increase, i) = size_of_saturatingusub(tokens, i);
        } else if (tokens.at(i) == "saturatingumul") {
            ++i;
            tie(increase, i) = size_of_saturatingumul(tokens, i);
        } else if (tokens.at(i) == "saturatingudiv") {
            ++i;
            tie(increase, i) = size_of_saturatingudiv(tokens, i);
        } else if (tokens.at(i) == "move") {
            ++i;
            tie(increase, i) = size_of_move(tokens, i);
        } else if (tokens.at(i) == "copy") {
            ++i;
            tie(increase, i) = size_of_copy(tokens, i);
        } else if (tokens.at(i) == "ptr") {
            ++i;
            tie(increase, i) = size_of_ptr(tokens, i);
        } else if (tokens.at(i) == "ptrlive") {
            ++i;
            tie(increase, i) = size_of_ptrlive(tokens, i);
        } else if (tokens.at(i) == "swap") {
            ++i;
            tie(increase, i) = size_of_swap(tokens, i);
        } else if (tokens.at(i) == "delete") {
            ++i;
            tie(increase, i) = size_of_delete(tokens, i);
        } else if (tokens.at(i) == "isnull") {
            ++i;
            tie(increase, i) = size_of_isnull(tokens, i);
        } else if (tokens.at(i) == "print") {
            ++i;
            tie(increase, i) = size_of_print(tokens, i);
        } else if (tokens.at(i) == "echo") {
            ++i;
            tie(increase, i) = size_of_echo(tokens, i);
        } else if (tokens.at(i) == "capture") {
            ++i;
            tie(increase, i) = size_of_capture(tokens, i);
        } else if (tokens.at(i) == "capturecopy") {
            ++i;
            tie(increase, i) = size_of_capturecopy(tokens, i);
        } else if (tokens.at(i) == "capturemove") {
            ++i;
            tie(increase, i) = size_of_capturemove(tokens, i);
        } else if (tokens.at(i) == "closure") {
            ++i;
            tie(increase, i) = size_of_closure(tokens, i);
        } else if (tokens.at(i) == "function") {
            ++i;
            tie(increase, i) = size_of_function(tokens, i);
        } else if (tokens.at(i) == "frame") {
            ++i;
            tie(increase, i) = size_of_frame(tokens, i);
        } else if (tokens.at(i) == "call") {
            ++i;
            tie(increase, i) = size_of_call(tokens, i);
        } else if (tokens.at(i) == "tailcall") {
            ++i;
            tie(increase, i) = size_of_tailcall(tokens, i);
        } else if (tokens.at(i) == "defer") {
            ++i;
            tie(increase, i) = size_of_defer(tokens, i);
        } else if (tokens.at(i) == "arg") {
            ++i;
            tie(increase, i) = size_of_arg(tokens, i);
        } else if (tokens.at(i) == "allocate_registers") {
            ++i;
            tie(increase, i) =
                size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);
        } else if (tokens.at(i) == "process") {
            ++i;
            tie(increase, i) = size_of_process(tokens, i);
        } else if (tokens.at(i) == "self") {
            ++i;
            tie(increase, i) = size_of_self(tokens, i);
        } else if (tokens.at(i) == "join") {
            ++i;
            tie(increase, i) = size_of_join(tokens, i);
        } else if (tokens.at(i) == "send") {
            ++i;
            tie(increase, i) = size_of_send(tokens, i);
        } else if (tokens.at(i) == "receive") {
            ++i;
            tie(increase, i) = size_of_receive(tokens, i);
        } else if (tokens.at(i) == "watchdog") {
            ++i;
            tie(increase, i) = size_of_watchdog(tokens, i);
        } else if (tokens.at(i) == "jump") {
            ++i;
            tie(increase, i) = size_of_jump(tokens, i);
        } else if (tokens.at(i) == "if") {
            ++i;
            tie(increase, i) = size_of_if(tokens, i);
        } else if (tokens.at(i) == "throw") {
            ++i;
            tie(increase, i) = size_of_throw(tokens, i);
        } else if (tokens.at(i) == "catch") {
            ++i;
            tie(increase, i) = size_of_catch(tokens, i);
        } else if (tokens.at(i) == "draw") {
            ++i;
            tie(increase, i) = size_of_pull(tokens, i);
        } else if (tokens.at(i) == "try") {
            ++i;
            tie(increase, i) = size_of_try(tokens, i);
        } else if (tokens.at(i) == "enter") {
            ++i;
            tie(increase, i) = size_of_enter(tokens, i);
        } else if (tokens.at(i) == "leave") {
            ++i;
            tie(increase, i) = size_of_leave(tokens, i);
        } else if (tokens.at(i) == "import") {
            ++i;
            tie(increase, i) = size_of_import(tokens, i);
        } else if (tokens.at(i) == "atom") {
            ++i;
            tie(increase, i) = size_of_atom(tokens, i);
        } else if (tokens.at(i) == "atomeq") {
            ++i;
            tie(increase, i) = size_of_atomeq(tokens, i);
        } else if (tokens.at(i) == "struct") {
            ++i;
            tie(increase, i) = size_of_struct(tokens, i);
        } else if (tokens.at(i) == "structinsert") {
            ++i;
            tie(increase, i) = size_of_structinsert(tokens, i);
        } else if (tokens.at(i) == "structremove") {
            ++i;
            tie(increase, i) = size_of_structremove(tokens, i);
        } else if (tokens.at(i) == "structkeys") {
            ++i;
            tie(increase, i) = size_of_structkeys(tokens, i);
        } else if (tokens.at(i) == "insert") {
            ++i;
            tie(increase, i) = size_of_insert(tokens, i);
        } else if (tokens.at(i) == "remove") {
            ++i;
            tie(increase, i) = size_of_remove(tokens, i);
        } else if (tokens.at(i) == "return") {
            ++i;
            tie(increase, i) = size_of_return(tokens, i);
        } else if (tokens.at(i) == "halt") {
            ++i;
            tie(increase, i) = size_of_halt(tokens, i);
        } else {
            throw viua::cg::lex::Invalid_syntax(
                tokens.at(i),
                ("failed to calculate size of: " + tokens.at(i).str()));
        }

        while (i < limit and tokens[i].str() != "\n") {
            ++i;
        }

        ++counted_instructions;

        bytes += increase;
    }

    return bytes;
}
auto calculate_bytecode_size2(TokenVector const& tokens) -> bytecode_size_type {
    return calculate_bytecode_size_of_first_n_instructions2(tokens,
                                                            tokens.size());
}
}}}  // namespace viua::cg::tools
