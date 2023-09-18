/*
 *  Copyright (C) 2023 Marek Marecki
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

#include <array>
#include <charconv>
#include <vector>

#include <viua/libs/assembler.h>
#include <viua/libs/lexer.h>
#include <viua/libs/parser.h>

namespace viua::libs::stage {
using namespace viua::libs::parser;
auto expand_delete(std::vector<ast::Instruction>& cooked,
                   ast::Instruction const& raw) -> void
{
    using namespace std::string_literals;
    auto synth        = ast::Instruction{};
    synth.opcode      = raw.opcode;
    synth.opcode.text = (raw.opcode.text.find("g.") == 0 ? "g." : "") + "move"s;
    synth.physical_index = raw.physical_index;

    synth.operands.push_back(raw.operands.front());
    synth.operands.push_back(raw.operands.front());

    synth.operands.front().ingredients.front().text = "void";
    synth.operands.front().ingredients.resize(1);

    cooked.push_back(synth);
}
auto expand_li(std::vector<ast::Instruction>& cooked,
               ast::Instruction const& each,
               bool const force_full) -> void
{
    auto const& raw_value = each.operands.at(1).ingredients.front();
    auto value            = uint64_t{};
    try {
        if (raw_value.text.find("0x") == 0) {
            value = std::stoull(raw_value.text, nullptr, 16);
        } else if (raw_value.text.find("0o") == 0) {
            value = std::stoull(raw_value.text, nullptr, 8);
        } else if (raw_value.text.find("0b") == 0) {
            value = std::stoull(raw_value.text, nullptr, 2);
        } else {
            value = std::stoull(raw_value.text);
        }
    } catch (std::invalid_argument const&) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{raw_value, Cause::Invalid_operand, "expected integer"}.add(
            each.opcode);
    }

    using viua::libs::assembler::to_loading_parts_unsigned;
    auto const [hi, lo]  = to_loading_parts_unsigned(value);
    auto const is_greedy = (each.opcode.text.find("g.") == 0);
    auto const full_form = each.operands.at(1).has_attr("full") or force_full;

    auto const is_unsigned = (raw_value.text.back() == 'u');

    constexpr auto LOW_24   = uint32_t{0x00ffffff};
    auto const fits_in_addi = ((lo & LOW_24) == lo);

    auto const needs_leader = full_form or hi or (not fits_in_addi);

    /*
     * When loading immediates we have several cases to consider:
     *
     *  - the immediate is short ie, occupies only lower 24-bits and thus fits
     *    fully in the immediate of an ADDI instruction
     *  - the immediate is long ie, has any of the high 40 bits set
     *
     * In the first case we could be done with a single ADDI, and (if we are not
     * loading addresses and need to take the pessimistic route to accommodate
     * the linker) sometimes we can.
     *
     * However, in most cases the LI pseudoinstruction must be expanded into two
     * real instructions:
     *
     *  - LUI to load the high word
     *  - LLI to load the low word
     *
     * LUI is necessary even if the long immediate being loaded only has lower
     * 32 bits set, because LLI needs a value to be already present in the
     * output register to determine the signedness of its output.
     *
     * In any case, for long immediates the sequence of
     *
     *      g.lui $x, <high-word>
     *      lli $x, <low-word>
     *
     * is emitted which is cheap and executed without releasing the virtual CPU.
     */
    if (needs_leader) {
        using namespace std::string_literals;
        auto synth        = each;
        synth.opcode.text = ((lo or is_greedy) ? "g.lui" : "lui");
        if (is_unsigned) {
            synth.opcode.text += 'u';
        }

        auto hi_literal = std::array<char, 2 + 8 + 1>{"0x"};
        std::to_chars(hi_literal.begin() + 2, hi_literal.end(), hi, 16);

        synth.operands.at(1).ingredients.front().text =
            std::string{hi_literal.data()};
        cooked.push_back(synth);
    }

    /*
     * In the second step we use LLI to load the lower word if we are dealing
     * with a long immediate; or ADDI if we have a short immediate to deal with.
     */
    if (needs_leader) {
        auto synth           = ast::Instruction{};
        synth.opcode         = each.opcode;
        synth.opcode.text    = (is_greedy ? "g.lli" : "lli");
        synth.physical_index = each.physical_index;

        auto const& lx = each.operands.front().ingredients.at(1);

        using viua::libs::lexer::TOKEN;
        {
            auto dst = ast::Operand{};
            dst.ingredients.push_back(lx.make_synth("$", TOKEN::RA_DIRECT));
            dst.ingredients.push_back(lx.make_synth(
                std::to_string(std::stoull(lx.text)), TOKEN::LITERAL_INTEGER));
            dst.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
            dst.ingredients.push_back(lx.make_synth("l", TOKEN::LITERAL_ATOM));

            synth.operands.push_back(dst);
        }
        {
            auto immediate = ast::Operand{};
            immediate.ingredients.push_back(
                lx.make_synth(std::to_string(lo), TOKEN::LITERAL_INTEGER));

            synth.operands.push_back(immediate);
        }

        cooked.push_back(synth);
    } else {
        using namespace std::string_literals;
        auto synth           = ast::Instruction{};
        synth.opcode         = each.opcode;
        synth.opcode.text    = (is_greedy ? "g." : "") + "addi"s;
        synth.physical_index = each.physical_index;
        if (is_unsigned) {
            synth.opcode.text += 'u';
        }

        synth.operands.push_back(each.operands.front());

        /*
         * If the first part of the load (the high 36 bits) was zero then it
         * means we don't have anything to add to so the source (left-hand side
         * operand) should be void ie, the default value.
         */
        synth.operands.push_back(each.operands.front());
        synth.operands.back().ingredients.front().text = "void";
        synth.operands.back().ingredients.resize(1);

        synth.operands.push_back(each.operands.back());
        synth.operands.back().ingredients.front().text = std::to_string(lo);

        cooked.push_back(synth);
    }
}
auto expand_if(std::vector<ast::Instruction>& cooked,
               ast::Instruction& each,
               std::map<size_t, size_t> l2p) -> void
{
    /*
     * The address to jump to is not cooked directly into the final
     * instruction sequence because we may have to try several times
     * before getting the "right" instruction sequence. Why? See the
     * comment for teh `branch_target' variable in the loop below.
     *
     * Basically, it boils down to the fact that:
     *
     * - we have to adjust the target instruction sequence based on the
     *   value of the index of the instruction to jump to
     * - we have to adjust the target index also by the difference
     *   between logical and physical instruction count
     */
    auto address_load             = std::vector<ast::Instruction>{};
    auto prev_address_load_length = address_load.size();
    constexpr auto MAX_SIZE_OF_LI = size_t{9};
    do {
        prev_address_load_length = address_load.size();
        address_load.clear();

        auto const branch_target =
            l2p.at(std::stoull(each.operands.back().to_string()));

        auto li = ast::Instruction{};
        {
            li.opcode         = each.opcode;
            li.opcode.text    = "g.li";
            li.physical_index = each.physical_index;

            using viua::libs::lexer::TOKEN;
            auto const& lx = each.operands.front().ingredients.front();
            {
                auto reg = ast::Operand{};
                reg.ingredients.push_back(lx.make_synth("$", TOKEN::RA_DIRECT));
                reg.ingredients.push_back(
                    lx.make_synth("253", TOKEN::LITERAL_INTEGER));
                reg.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
                reg.ingredients.push_back(
                    lx.make_synth("l", TOKEN::LITERAL_ATOM));
                li.operands.push_back(reg);
            }
            {
                auto br = ast::Operand{};
                br.ingredients.push_back(
                    lx.make_synth(std::to_string(branch_target) + 'u',
                                  TOKEN::LITERAL_INTEGER));
                li.operands.push_back(br);
            }
        }

        /*
         * Try cooking the instruction sequence to see what amount of
         * instructions we will get. If we get the same instruction
         * sequence twice - we got our match and the loop can end. The
         * downside of this brute-force algorithm is that we may have to
         * cook the same code twice in some cases.
         */
        expand_li(address_load, li);
    } while (address_load.size() < MAX_SIZE_OF_LI
             and prev_address_load_length != address_load.size());
    std::copy(
        address_load.begin(), address_load.end(), std::back_inserter(cooked));

    using viua::libs::lexer::TOKEN;
    auto reg = each.operands.back();
    each.operands.pop_back();

    auto const lx = reg.ingredients.front();
    reg.ingredients.pop_back();

    reg.ingredients.push_back(lx.make_synth("$", TOKEN::RA_DIRECT));
    reg.ingredients.push_back(lx.make_synth("253", TOKEN::LITERAL_INTEGER));
    reg.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
    reg.ingredients.push_back(lx.make_synth("l", TOKEN::LITERAL_ATOM));

    each.operands.push_back(reg);
    cooked.push_back(std::move(each));
}
auto expand_memory_access(std::vector<ast::Instruction>& cooked,
                          ast::Instruction const& raw) -> void
{
    using namespace std::string_literals;
    auto synth = ast::Instruction{};

    auto opcode_view = std::string_view{raw.opcode.text};
    auto greedy      = opcode_view.starts_with("g.");
    if (greedy) {
        opcode_view.remove_prefix(2);
    }

    synth.opcode         = raw.opcode;
    synth.opcode.text    = opcode_view;
    synth.opcode.text[1] = 'm';
    if (opcode_view[0] == 'a') {
        switch (synth.opcode.text.back()) {
        case 'a':
            synth.opcode.text = "ama";
            break;
        case 'd':
            synth.opcode.text = "amd";
            break;
        default:
            abort();
        }
    }
    synth.physical_index = raw.physical_index;

    synth.operands.push_back(raw.operands.at(0));
    synth.operands.push_back(raw.operands.at(0));
    synth.operands.push_back(raw.operands.at(1));
    synth.operands.push_back(raw.operands.at(2));

    auto const unit = opcode_view[0] == 'a' ? opcode_view[2] : opcode_view[1];
    switch (unit) {
    case 'b':
        synth.operands.front().ingredients.front().text = "0";
        break;
    case 'h':
        synth.operands.front().ingredients.front().text = "1";
        break;
    case 'w':
        synth.operands.front().ingredients.front().text = "2";
        break;
    case 'd':
        synth.operands.front().ingredients.front().text = "3";
        break;
    case 'q':
        synth.operands.front().ingredients.front().text = "4";
        break;
    default:
        abort();
    }
    synth.operands.front().ingredients.resize(1);

    if (greedy) {
        synth.opcode.text = ("g." + synth.opcode.text);
    }

    cooked.push_back(synth);
}
}  // namespace viua::libs::stage
