/*
 *  Copyright (C) 2016, 2017 Marek Marecki
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
using TokenVector = vector<Token>;
using bytecode_size_type = viua::internals::types::bytecode_size;


namespace viua {
    namespace cg {
        namespace tools {
            static auto size_of_register_index_operand_with_rs_type(const TokenVector& tokens,
                                                                    TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = 0;

                if (tokens.at(i) == "void" or tokens.at(i) == "true" or tokens.at(i) == "false") {
                    calculated_size += sizeof(viua::internals::types::byte);
                    ++i;
                } else if (tokens.at(i).str().at(0) == '%' and str::isnum(tokens.at(i).str().substr(1))) {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::RegisterSets);
                    calculated_size += sizeof(viua::internals::types::register_index);
                    ++i;

                    if (tokens.at(i) == "current" or tokens.at(i) == "local" or tokens.at(i) == "static" or
                        tokens.at(i) == "global") {
                        ++i;
                    }
                } else if (tokens.at(i).str().at(0) == '@') {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::RegisterSets);
                    calculated_size += sizeof(viua::internals::types::register_index);
                    ++i;

                    if (tokens.at(i) == "current" or tokens.at(i) == "local" or tokens.at(i) == "static" or
                        tokens.at(i) == "global") {
                        ++i;
                    }
                } else if (tokens.at(i).str().at(0) == '*') {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::RegisterSets);
                    calculated_size += sizeof(viua::internals::types::register_index);
                    ++i;

                    if (tokens.at(i) == "current" or tokens.at(i) == "local" or tokens.at(i) == "static" or
                        tokens.at(i) == "global") {
                        ++i;
                    }
                } else {
                    throw viua::cg::lex::InvalidSyntax(tokens.at(i),
                                                       ("invalid operand token: " + tokens.at(i).str()));
                }

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_register_index_operand(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = 0;

                if (tokens.at(i) == "static" or tokens.at(i) == "local" or tokens.at(i) == "global") {
                    ++i;
                }

                if (tokens.at(i) == "void" or tokens.at(i) == "true" or tokens.at(i) == "false") {
                    calculated_size += sizeof(viua::internals::types::byte);
                    ++i;
                } else if (tokens.at(i).str().at(0) == '%' and str::isnum(tokens.at(i).str().substr(1))) {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::RegisterSets);
                    calculated_size += sizeof(viua::internals::types::register_index);
                    ++i;
                } else if (tokens.at(i).str().at(0) == '@') {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::RegisterSets);
                    calculated_size += sizeof(viua::internals::types::register_index);
                    ++i;
                } else if (tokens.at(i).str().at(0) == '*') {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::RegisterSets);
                    calculated_size += sizeof(viua::internals::types::register_index);
                    ++i;
                } else {
                    throw viua::cg::lex::InvalidSyntax(tokens.at(i),
                                                       ("invalid operand token: " + tokens.at(i).str()));
                }

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }

            static auto size_of_instruction_with_no_operands(const TokenVector&, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);
                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_with_one_ri_operand(const TokenVector& tokens,
                                                                TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_with_one_ri_operand_with_rs_type(const TokenVector& tokens,
                                                                             TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_with_two_ri_operands_with_rs_types(const TokenVector& tokens,
                                                                               TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_with_two_ri_operands(const TokenVector& tokens,
                                                                 TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_with_three_ri_operands_with_rs_types(const TokenVector& tokens,
                                                                                 TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for 1st source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for 2nd source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_with_four_ri_operands_with_rs_types(const TokenVector& tokens,
                                                                                TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for 1st source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for 2nd source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for 3rd source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_instruction_with_three_ri_operands(const TokenVector& tokens,
                                                                   TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

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
            static auto size_of_instruction_alu(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }

            static auto size_of_binary_literal_operand(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);
                calculated_size += sizeof(viua::internals::types::bits_size);

                auto literal = tokens.at(i).str().substr(2);
                ++i;

                /*
                 * Size is first multiplied by 1, because every character of the
                 * string may be encoded on one bit.
                 * Then the result is divided by 8.0 because a byte has 8 bits, and
                 * the bytecode is encoded using bytes.
                 */
                auto size = (assembler::operands::normalise_binary_literal(literal).size() / 8);
                calculated_size += size;

                return tuple<bytecode_size_type, decltype(i)>{calculated_size, i};
            }

            static auto size_of_octal_literal_operand(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);
                calculated_size += sizeof(viua::internals::types::bits_size);

                auto literal = tokens.at(i).str();
                ++i;

                calculated_size += (assembler::operands::normalise_binary_literal(
                                        assembler::operands::octal_to_binary_literal(literal))
                                        .size() /
                                    8);

                return tuple<bytecode_size_type, decltype(i)>{calculated_size, i};
            }

            static auto size_of_hexadecimal_literal_operand(const TokenVector& tokens,
                                                            TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);
                calculated_size += sizeof(viua::internals::types::bits_size);

                auto literal = tokens.at(i).str();
                ++i;

                calculated_size += (assembler::operands::normalise_binary_literal(
                                        assembler::operands::hexadecimal_to_binary_literal(literal))
                                        .size() /
                                    8);

                return tuple<bytecode_size_type, decltype(i)>{calculated_size, i};
            }

            static auto size_of_nop(const TokenVector& v, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_no_operands(v, i);
            }
            static auto size_of_izero(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_istore(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = 0;
                tie(calculated_size, i) = size_of_instruction_with_one_ri_operand(tokens, i);

                calculated_size += sizeof(viua::internals::types::byte);
                calculated_size += sizeof(viua::internals::types::plain_int);
                ++i;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_iinc(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_idec(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_fstore(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                calculated_size += sizeof(viua::internals::types::plain_float);
                ++i;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_itof(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_ftoi(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_stoi(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_stof(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_add(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_sub(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_mul(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_div(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_lt(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_lte(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_gt(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_gte(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_eq(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_alu(tokens, i);
            }
            static auto size_of_string(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                ++calculated_size;  // for operand type
                calculated_size +=
                    tokens.at(i++).str().size() + 1 - 2;  // +1 for null terminator, -2 for quotes

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_streq(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands(tokens, i);
            }

            static auto size_of_text(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                if (tokens.at(i).str().at(0) == '*' or tokens.at(i).str().at(0) == '%') {
                    tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                    calculated_size += size_increment;
                } else {
                    ++calculated_size;  // for operand type
                    calculated_size +=
                        tokens.at(i++).str().size() + 1 - 2;  // +1 for null terminator, -2 for quotes
                }

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_texteq(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_textat(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_textsub(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_four_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_textlength(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_textcommonprefix(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_textcommonsuffix(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_textconcat(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }

            static auto size_of_vec(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for pack start register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for pack count register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_vinsert(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for position index
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_vpush(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_vpop(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_vat(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_vlen(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_bool(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_not(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_and(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_or(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_bits(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                if (tokens.at(i).str().at(0) == '*' or tokens.at(i).str().at(0) == '%') {
                    tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
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
            static auto size_of_bitand(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_bitor(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_bitnot(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_bitxor(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_bitat(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_bitset(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_shl(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_shr(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_ashl(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_ashr(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_rol(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_ror(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_wrapincrement(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_wrapdecrement(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_wrapadd(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_wrapmul(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_wrapdiv(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_move(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_copy(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_ptr(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_swap(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_delete(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_isnull(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_ress(const TokenVector&, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);
                calculated_size += sizeof(viua::internals::types::registerset_type_marker);
                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_print(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_echo(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_capture(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for inside-closure register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_capturecopy(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for inside-closure register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_capturemove(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for inside-closure register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_closure(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                calculated_size += tokens.at(i).str().size() + 1;
                ++i;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_function(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                calculated_size += tokens.at(i).str().size() + 1;
                ++i;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_frame(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands(tokens, i);
            }
            static auto size_of_param(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_pamv(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_call(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                if (tokens.at(i).str().at(0) == '*' or tokens.at(i).str().at(0) == '%') {
                    tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                    calculated_size += size_increment;
                } else {
                    calculated_size += tokens.at(i).str().size() + 1;
                    ++i;
                }

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_tailcall(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                if (tokens.at(i).str().at(0) == '*' or tokens.at(i).str().at(0) == '%') {
                    decltype(calculated_size) size_increment = 0;
                    tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                    calculated_size += size_increment;
                } else {
                    calculated_size += tokens.at(i).str().size() + 1;
                    ++i;
                }

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_defer(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_tailcall(tokens, i);
            }
            static auto size_of_arg(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for source register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_argc(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);
            }
            static auto size_of_process(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                calculated_size += tokens.at(i).str().size() + 1;
                ++i;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_self(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);
            }
            static auto size_of_join(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = 0;
                tie(calculated_size, i) = size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);

                if (str::is_timeout_literal(tokens.at(i))) {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::types::timeout);
                    ++i;
                } else {
                    throw viua::cg::lex::InvalidSyntax(tokens.at(i), "invalid timeout token in 'join'");
                }

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_send(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_receive(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = 0;
                tie(calculated_size, i) = size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);

                if (str::is_timeout_literal(tokens.at(i))) {
                    calculated_size += sizeof(viua::internals::types::byte);
                    calculated_size += sizeof(viua::internals::types::timeout);
                    ++i;
                } else {
                    throw viua::cg::lex::InvalidSyntax(tokens.at(i), "invalid timeout token in 'receive'");
                }

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_watchdog(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                calculated_size += tokens.at(i).str().size() + 1;
                ++i;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_jump(const TokenVector&, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);
                calculated_size += sizeof(bytecode_size_type);
                ++i;
                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_if(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;
                // for source register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                calculated_size += 2 * sizeof(bytecode_size_type);
                i += 2;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_throw(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);
            }
            static auto size_of_catch(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);

                calculated_size +=
                    tokens.at(i++).str().size() + 1 - 2;             // +1 for null terminator, -2 for quotes
                calculated_size += tokens.at(i++).str().size() + 1;  // +1 for null terminator

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_pull(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);
            }
            static auto size_of_try(const TokenVector&, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);
                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_enter(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);

                calculated_size += tokens.at(i++).str().size() + 1;  // +1 for null terminator

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_leave(const TokenVector&, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);
                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_import(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);

                calculated_size +=
                    tokens.at(i++).str().size() + 1 - 2;  // +1 for null terminator, -2 for quotes

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_class(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                calculated_size += tokens.at(i++).str().size() + 1;  // +1 for null terminator

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_prototype(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand(tokens, i);
                calculated_size += size_increment;

                calculated_size += tokens.at(i++).str().size() + 1;  // +1 for null terminator

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_derive(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for class name, +1 for null-terminator
                calculated_size += (tokens.at(i).str().size() + 1);
                ++i;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_attach(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for real name of aliased function, +1 for null-terminator
                calculated_size += (tokens.at(i).str().size() + 1);
                ++i;

                // for name of the alias, +1 for null-terminator
                calculated_size += (tokens.at(i).str().size() + 1);
                ++i;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_register(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand_with_rs_type(tokens, i);
            }

            static auto size_of_atom(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                calculated_size +=
                    tokens.at(i++).str().size() + 1 - 2;  // +1 for null terminator, -2 for quotes

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_atomeq(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }

            static auto size_of_struct(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_one_ri_operand(tokens, i);
            }
            static auto size_of_structinsert(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_structremove(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_structkeys(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_two_ri_operands_with_rs_types(tokens, i);
            }

            static auto size_of_new(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                // for class name, +1 for null-terminator
                calculated_size += (tokens.at(i).str().size() + 1);
                ++i;

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_msg(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode

                decltype(calculated_size) size_increment = 0;

                // for target register
                tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                calculated_size += size_increment;

                if (tokens.at(i).str().at(0) == '*' or tokens.at(i).str().at(0) == '%') {
                    tie(size_increment, i) = size_of_register_index_operand_with_rs_type(tokens, i);
                    calculated_size += size_increment;
                } else {
                    calculated_size += tokens.at(i).str().size() + 1;
                    ++i;
                }

                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_insert(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_remove(const TokenVector& tokens, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                return size_of_instruction_with_three_ri_operands_with_rs_types(tokens, i);
            }
            static auto size_of_return(const TokenVector&, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size =
                    sizeof(viua::internals::types::byte);  // start with the size of a single opcode
                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }
            static auto size_of_halt(const TokenVector&, TokenVector::size_type i)
                -> tuple<bytecode_size_type, decltype(i)> {
                bytecode_size_type calculated_size = sizeof(viua::internals::types::byte);
                return tuple<bytecode_size_type, decltype(i)>(calculated_size, i);
            }

            bytecode_size_type calculate_bytecode_size_of_first_n_instructions2(
                const TokenVector& tokens,
                const std::remove_reference<decltype(tokens)>::type::size_type instructions_counter) {
                bytecode_size_type bytes = 0;

                const auto limit = tokens.size();
                std::remove_const<decltype(instructions_counter)>::type counted_instructions = 0;
                for (TokenVector::size_type i = 0;
                     (i < limit) and (counted_instructions < instructions_counter); ++i) {
                    auto token = tokens.at(i);

                    if (token == ".function:" or token == ".closure:" or token == ".block:" or
                        token == ".mark:" or token == ".import:" or token == ".signature:" or
                        token == ".bsignature:" or token == ".unused:") {
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

                    bytecode_size_type increase = 0;
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
                    } else if (tokens.at(i) == "wrapmul") {
                        ++i;
                        tie(increase, i) = size_of_wrapmul(tokens, i);
                    } else if (tokens.at(i) == "wrapdiv") {
                        ++i;
                        tie(increase, i) = size_of_wrapdiv(tokens, i);
                    } else if (tokens.at(i) == "move") {
                        ++i;
                        tie(increase, i) = size_of_move(tokens, i);
                    } else if (tokens.at(i) == "copy") {
                        ++i;
                        tie(increase, i) = size_of_copy(tokens, i);
                    } else if (tokens.at(i) == "ptr") {
                        ++i;
                        tie(increase, i) = size_of_ptr(tokens, i);
                    } else if (tokens.at(i) == "swap") {
                        ++i;
                        tie(increase, i) = size_of_swap(tokens, i);
                    } else if (tokens.at(i) == "delete") {
                        ++i;
                        tie(increase, i) = size_of_delete(tokens, i);
                    } else if (tokens.at(i) == "isnull") {
                        ++i;
                        tie(increase, i) = size_of_isnull(tokens, i);
                    } else if (tokens.at(i) == "ress") {
                        ++i;
                        tie(increase, i) = size_of_ress(tokens, i);
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
                    } else if (tokens.at(i) == "param") {
                        ++i;
                        tie(increase, i) = size_of_param(tokens, i);
                    } else if (tokens.at(i) == "pamv") {
                        ++i;
                        tie(increase, i) = size_of_pamv(tokens, i);
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
                    } else if (tokens.at(i) == "argc") {
                        ++i;
                        tie(increase, i) = size_of_argc(tokens, i);
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
                    } else if (tokens.at(i) == "class") {
                        ++i;
                        tie(increase, i) = size_of_class(tokens, i);
                    } else if (tokens.at(i) == "prototype") {
                        ++i;
                        tie(increase, i) = size_of_prototype(tokens, i);
                    } else if (tokens.at(i) == "derive") {
                        ++i;
                        tie(increase, i) = size_of_derive(tokens, i);
                    } else if (tokens.at(i) == "attach") {
                        ++i;
                        tie(increase, i) = size_of_attach(tokens, i);
                    } else if (tokens.at(i) == "register") {
                        ++i;
                        tie(increase, i) = size_of_register(tokens, i);
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
                    } else if (tokens.at(i) == "new") {
                        ++i;
                        tie(increase, i) = size_of_new(tokens, i);
                    } else if (tokens.at(i) == "msg") {
                        ++i;
                        tie(increase, i) = size_of_msg(tokens, i);
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
                        throw viua::cg::lex::InvalidSyntax(
                            tokens.at(i), ("failed to calculate size of: " + tokens.at(i).str()));
                    }

                    while (i < limit and tokens[i].str() != "\n") {
                        ++i;
                    }

                    ++counted_instructions;

                    bytes += increase;
                }

                return bytes;
            }
            bytecode_size_type calculate_bytecode_size2(const TokenVector& tokens) {
                return calculate_bytecode_size_of_first_n_instructions2(tokens, tokens.size());
            }
        }  // namespace tools
    }      // namespace cg
}  // namespace viua
