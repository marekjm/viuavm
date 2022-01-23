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

#include <elf.h>
#include <endian.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/arch/ops.h>
#include <viua/libs/assembler.h>
#include <viua/libs/errors/compile_time.h>
#include <viua/libs/lexer.h>
#include <viua/libs/parser.h>
#include <viua/support/string.h>
#include <viua/support/tty.h>
#include <viua/support/vector.h>


constexpr auto DEBUG_LEX       = false;
constexpr auto DEBUG_EXPANSION = false;


namespace {
auto save_string(std::vector<uint8_t>& strings, std::string_view const data)
    -> size_t
{
    auto const data_size = htole64(static_cast<uint64_t>(data.size()));
    strings.resize(strings.size() + sizeof(data_size));
    memcpy((strings.data() + strings.size() - sizeof(data_size)),
           &data_size,
           sizeof(data_size));

    auto const saved_location = strings.size();
    std::copy(data.begin(), data.end(), std::back_inserter(strings));

    return saved_location;
}
auto save_fn_address(std::vector<uint8_t>& strings, std::string_view const fn)
    -> size_t
{
    auto const fn_size = htole64(static_cast<uint64_t>(fn.size()));
    strings.resize(strings.size() + sizeof(fn_size));
    memcpy((strings.data() + strings.size() - sizeof(fn_size)),
           &fn_size,
           sizeof(fn_size));

    auto const saved_location = strings.size();
    std::copy(fn.begin(), fn.end(), std::back_inserter(strings));

    auto const fn_addr = uint64_t{0};
    strings.resize(strings.size() + sizeof(fn_addr));
    memcpy((strings.data() + strings.size() - sizeof(fn_addr)),
           &fn_addr,
           sizeof(fn_addr));

    return saved_location;
}
auto patch_fn_address(std::vector<uint8_t>& strings,
                      size_t const fn_offset,
                      uint64_t fn_addr) -> void
{
    auto fn_size = uint64_t{};
    memcpy(&fn_size,
           (strings.data() + fn_offset - sizeof(fn_size)),
           sizeof(fn_size));
    fn_size = le64toh(fn_size);

    fn_addr = htole64(fn_addr);
    memcpy((strings.data() + fn_offset + fn_size), &fn_addr, sizeof(fn_addr));
}
}  // anonymous namespace

namespace {
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
               ast::Instruction const& each) -> void
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
        throw Error{raw_value, Cause::Invalid_operand, "expected integer"};
    }

    using viua::libs::assembler::to_loading_parts_unsigned;
    auto parts            = to_loading_parts_unsigned(value);
    auto const base       = parts.second.first.first;
    auto const multiplier = parts.second.first.second;
    auto const is_greedy  = (each.opcode.text.find("g.") == 0);
    auto const full_form  = each.operands.at(1).has_attr("full");

    auto const is_unsigned = (raw_value.text.back() == 'u');

    /*
     * Only use the luiu instruction of there's a reason to ie, if some
     * of the highest 36 bits are set. Otherwise, the lui is just
     * overhead.
     */
    if (parts.first or full_form) {
        using namespace std::string_literals;
        auto synth = each;
        synth.opcode.text =
            ((multiplier or base or is_greedy) ? "g.lui" : "lui");
        if (is_unsigned) {
            // FIXME loading signed values is ridiculously expensive and always
            // takes the most pessmisitic route - write a signed version of the
            // algorithm
            synth.opcode.text += 'u';
        }
        synth.operands.at(1).ingredients.front().text =
            std::to_string(parts.first);
        cooked.push_back(synth);
    }

    if ((multiplier != 0) or full_form) {
        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.addiu";
            synth.physical_index = each.physical_index;

            auto const& lx = each.operands.front().ingredients.at(1);

            using viua::libs::lexer::TOKEN;
            {
                auto dst = ast::Operand{};
                dst.ingredients.push_back(lx.make_synth("$", TOKEN::RA_DIRECT));
                dst.ingredients.push_back(
                    lx.make_synth(std::to_string(std::stoull(lx.text) + 1),
                                  TOKEN::LITERAL_INTEGER));
                dst.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
                dst.ingredients.push_back(
                    lx.make_synth("l", TOKEN::LITERAL_ATOM));

                synth.operands.push_back(dst);
            }
            {
                auto src = ast::Operand{};
                src.ingredients.push_back(
                    lx.make_synth("void", TOKEN::RA_VOID));

                synth.operands.push_back(src);
            }
            {
                auto immediate = ast::Operand{};
                immediate.ingredients.push_back(lx.make_synth(
                    std::to_string(base), TOKEN::LITERAL_INTEGER));

                synth.operands.push_back(immediate);
            }

            cooked.push_back(synth);
        }
        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.addiu";
            synth.physical_index = each.physical_index;

            auto const& lx = each.operands.front().ingredients.at(1);

            using viua::libs::lexer::TOKEN;
            {
                auto dst = ast::Operand{};
                dst.ingredients.push_back(lx.make_synth("$", TOKEN::RA_DIRECT));
                dst.ingredients.push_back(
                    lx.make_synth(std::to_string(std::stoull(lx.text) + 2),
                                  TOKEN::LITERAL_INTEGER));
                dst.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
                dst.ingredients.push_back(
                    lx.make_synth("l", TOKEN::LITERAL_ATOM));

                synth.operands.push_back(dst);
            }
            {
                auto src = ast::Operand{};
                src.ingredients.push_back(
                    lx.make_synth("void", TOKEN::RA_VOID));

                synth.operands.push_back(src);
            }
            {
                /*
                 * The multiplier may be zero if the full form of the expansion
                 * is requested eg, for an address load. Without this additional
                 * check the result of the expansion would be incorrect because
                 * the multiplier would zero the low part of the integer.
                 */
                auto const imm_value = (multiplier != 0) ? multiplier : 1;
                auto immediate       = ast::Operand{};
                immediate.ingredients.push_back(lx.make_synth(
                    std::to_string(imm_value), TOKEN::LITERAL_INTEGER));

                synth.operands.push_back(immediate);
            }

            cooked.push_back(synth);
        }
        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.mul";
            synth.physical_index = each.physical_index;

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 1);

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 1);

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 2);

            cooked.push_back(synth);
        }

        auto const remainder = parts.second.second;
        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.addiu";
            synth.physical_index = each.physical_index;

            auto const& lx = each.operands.front().ingredients.at(1);

            using viua::libs::lexer::TOKEN;
            {
                auto dst = ast::Operand{};
                dst.ingredients.push_back(lx.make_synth("$", TOKEN::RA_DIRECT));
                dst.ingredients.push_back(
                    lx.make_synth(std::to_string(std::stoull(lx.text) + 2),
                                  TOKEN::LITERAL_INTEGER));
                dst.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
                dst.ingredients.push_back(
                    lx.make_synth("l", TOKEN::LITERAL_ATOM));

                synth.operands.push_back(dst);
            }
            {
                auto src = ast::Operand{};
                src.ingredients.push_back(
                    lx.make_synth("void", TOKEN::RA_VOID));

                synth.operands.push_back(src);
            }
            {
                auto immediate = ast::Operand{};
                immediate.ingredients.push_back(lx.make_synth(
                    std::to_string(remainder), TOKEN::LITERAL_INTEGER));

                synth.operands.push_back(immediate);
            }

            cooked.push_back(synth);
        }
        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.add";
            synth.physical_index = each.physical_index;

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 1);

            synth.operands.push_back(synth.operands.back());

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 2);

            cooked.push_back(synth);
        }
        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.add";
            synth.physical_index = each.physical_index;

            synth.operands.push_back(each.operands.front());
            synth.operands.push_back(each.operands.front());

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 1);

            cooked.push_back(synth);
        }

        {
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = "g.delete";
            synth.physical_index = each.physical_index;

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 1);

            expand_delete(cooked, synth);
        }
        {
            using namespace std::string_literals;
            auto synth           = ast::Instruction{};
            synth.opcode         = each.opcode;
            synth.opcode.text    = (is_greedy ? "g." : "") + "delete"s;
            synth.physical_index = each.physical_index;

            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.at(1).text = std::to_string(
                std::stoul(synth.operands.back().ingredients.at(1).text) + 2);

            expand_delete(cooked, synth);
        }
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
         *
         * Otherwise, we should increase the value stored by the lui
         * instruction.
         */
        if (parts.first == 0) {
            synth.operands.push_back(each.operands.front());
            synth.operands.back().ingredients.front().text = "void";
            synth.operands.back().ingredients.resize(1);
        } else {
            synth.operands.push_back(each.operands.front());
        }

        synth.operands.push_back(each.operands.back());
        synth.operands.back().ingredients.front().text = std::to_string(base);

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
auto expand_pseudoinstructions(std::vector<ast::Instruction> raw,
                               std::map<std::string, size_t> const& fn_offsets)
    -> std::vector<ast::Instruction>
{
    auto const immediate_signed_arithmetic = std::set<std::string>{
        "addi",
        "g.addi",
        "subi",
        "g.subi",
        "muli",
        "g.muli",
        "divi",
        "g.divi",
    };

    /*
     * We need to maintain accurate mapping of logical to physical instructions
     * because what the programmer sees is not necessarily what the VM will be
     * executing. Pseudoinstructions are expanded into sequences of real
     * instructions and this changes effective addresses (ie, instruction
     * indexes) which are used as branch targets.
     *
     * A single logical instruction can expand into as many as 9 physical
     * instructions! The worst (and most common) such pseudoinstruction is the
     * li -- Loading Integer constants. It is used to store integers in
     * registers, as well as addresses for calls, jumps, and ifs.
     */
    auto physical_ops       = size_t{0};
    auto logical_ops        = size_t{0};
    auto branch_ops_baggage = size_t{0};
    auto l2p                = std::map<size_t, size_t>{};

    /*
     * First, we cook the sequence of raw instructions expanding non-control
     * flow instructions and building the mapping of logical to physical
     * instruction indexes.
     *
     * We do it this way because even an early branch can jump to the very last
     * instruction in a function, but we can't predict the future so we do not
     * know at this point how many PHYSICAL instructions are there between the
     * branch and its target -- this means that we do not know how to adjust the
     * logical target of the branch.
     *
     * What we can do, though, is to accrue the "instruction cost" of expanding
     * the branches and adjust the physical instruction indexes by this value. I
     * think this may be brittle and error-prone but so far it works... Let's
     * mark this part as HERE BE DRAGONS to be easier to find in the future.
     */
    auto cooked = std::vector<ast::Instruction>{};
    for (auto& each : raw) {
        physical_ops = each.physical_index;
        l2p.insert({physical_ops, logical_ops + branch_ops_baggage});

        // FIXME remove checking for "g.li" here to test errors with synthesized
        // instructions
        if (each.opcode == "li" or each.opcode == "g.li") {
            expand_li(cooked, each);
        } else if (each.opcode == "delete" or each.opcode == "g.delete") {
            expand_delete(cooked, each);
        } else if (each.opcode == "call") {
            /*
             * Call instructions expansion is simple.
             *
             * First, a li pseudoinstruction containing the offset of the
             * function in the function table is synthesized and expanded. The
             * call pseudoinstruction is then replaced by a real call
             * instruction in D format - first register tells it where to put
             * the return value, and the second tells it from which register to
             * take the function table offset value.
             *
             * If the return register is not void, it is used by the li
             * pseudoinstruction as base. If it is void, the li
             * pseudoinstruction has a base register allocated from a free
             * range.
             *
             * In effect, the following pseudoinstruction:
             *
             *      call $1, foo
             *
             * ...is expanded into this sequence:
             *
             *      li $1, fn_tbl_offset(foo)
             *      call $1, $1
             */
            auto const ret = each.operands.front();
            auto fn_offset = ret;
            if (ret.ingredients.front() == "void") {
                /*
                 * If the return register is void we need a completely synthetic
                 * register to store the function offset. The li
                 * pseudoinstruction uses at most three registers to do its job
                 * so we can use register 253 as the base to avoid disturbing
                 * user code.
                 */
                fn_offset = ast::Operand{};

                using viua::libs::lexer::TOKEN;
                auto const& lx = ret.ingredients.front();
                fn_offset.ingredients.push_back(
                    lx.make_synth("$", TOKEN::RA_DIRECT));
                fn_offset.ingredients.push_back(
                    lx.make_synth("253", TOKEN::LITERAL_INTEGER));
                fn_offset.ingredients.push_back(lx.make_synth(".", TOKEN::DOT));
                fn_offset.ingredients.push_back(
                    lx.make_synth("l", TOKEN::LITERAL_ATOM));
            }

            /*
             * Synthesize loading the function offset first. It will emit a
             * sequence of instructions that will load the offset of the
             * function, which will then be used by the call instruction to
             * invoke the function.
             */
            auto li = ast::Instruction{};
            {
                li.opcode      = each.opcode;
                li.opcode.text = "g.li";

                li.operands.push_back(fn_offset);
                li.operands.push_back(ast::Operand{});

                auto const fn_name = each.operands.back().ingredients.front();
                if (fn_offsets.count(fn_name.text) == 0) {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;
                    throw Error{fn_name, Cause::Call_to_undefined_function};
                }

                auto const fn_off = fn_offsets.at(fn_name.text);
                li.operands.back().ingredients.push_back(fn_name.make_synth(
                    std::to_string(fn_off) + 'u',
                    viua::libs::lexer::TOKEN::LITERAL_INTEGER));

                li.physical_index = each.physical_index;
            }
            expand_li(cooked, li);

            /*
             * Then, synthesize the actual call instruction. This means
             * simply replacing the function name with the register
             * containing its loaded offset, ie, changing this code:
             *
             *      call $42, foo
             *
             * to this:
             *
             *      call $42, $42
             */
            auto call = ast::Instruction{};
            {
                call.opcode = each.opcode;
                call.operands.push_back(ret);
                call.operands.push_back(fn_offset);
                call.physical_index = each.physical_index;
            }
            cooked.push_back(call);
        } else if (each.opcode == "return") {
            if (each.operands.empty()) {
                /*
                 * Return instruction takes a single register operand, but this
                 * operand can be omitted. As in C, if omitted it defaults to
                 * void.
                 */
                auto operand = ast::Operand{};
                operand.ingredients.push_back(each.opcode);
                operand.ingredients.back().text = "void";
                each.operands.push_back(operand);
            }

            cooked.push_back(std::move(each));
        } else if (immediate_signed_arithmetic.count(each.opcode.text)) {
            if (each.operands.back().ingredients.back().text.back() == 'u') {
                each.opcode.text += 'u';
            }
            cooked.push_back(std::move(each));
        } else {
            /*
             * Real instructions should be pushed without any modification.
             */
            cooked.push_back(std::move(each));
        }

        logical_ops += (cooked.size() - logical_ops);

        /*
         * I think this part of the code might break if the value crosses a
         * threshold at which the alhorithm governing expansion of li chooses a
         * different instruction sequence. This code is a potential FIXME.
         *
         * Maybe we should do a second pass in which the actual costs are
         * calculated. After the whole sequence is analysed we know how many
         * physical instructions are there. Then, we know that eg, Nth logical
         * instruction will be converted into Mth physical instruction address.
         * If they have different costs we would have to adjust the mappings for
         * all instructions after that jump.
         *
         * But... this requires the target of the branch just analysed to have
         * to be recalculated because the physical instrution's layout just
         * changed. Damn, what a brain-dead design was chosen for the li
         * expansion. Costant size would be much better.
         * Another choice is to never REDUCE the cost, but insert a bunch of
         * g.noop's as padding. Yeah, that could work, I guess.
         */
        using viua::libs::assembler::li_cost;
        if (each.opcode == "if" or each.opcode == "g.if") {
            branch_ops_baggage +=
                li_cost(std::stoull(each.operands.back().to_string()));
        } else if (each.opcode == "jump" or each.opcode == "g.jump") {
            branch_ops_baggage +=
                li_cost(std::stoull(each.operands.back().to_string()));
        }
    }

    auto baked = std::vector<ast::Instruction>{};
    for (auto& each : cooked) {
        if (each.opcode == "if" or each.opcode == "g.if") {
            expand_if(baked, each, l2p);
        } else if (each.opcode == "jump" or each.opcode == "g.jump") {
            /*
             * Jumps can be safely rewritten as ifs with a void condition
             * register. There is no reason not to do this, frankly, since
             * taking a void register as an operand is perfectly valid and an
             * established practice in the ISA -- void acts as a sort of a
             * default-value register. Thanks RISC-V for the idea of hardwired
             * r0!
             */
            auto as_if           = ast::Instruction{};
            as_if.opcode         = each.opcode;
            as_if.opcode.text    = (each.opcode == "jump") ? "if" : "g.if";
            as_if.physical_index = each.physical_index;

            {
                /*
                 * The condition operand for the emitted if is just a void. This
                 * is interpreted by the VM as a default value which in this
                 * case means "take the branch".
                 */
                auto condition = ast::Operand{};
                auto const& lx = each.operands.front().ingredients.front();
                condition.ingredients.push_back(
                    lx.make_synth("void", viua::libs::lexer::TOKEN::RA_VOID));
                as_if.operands.push_back(condition);
            }

            /*
             * if is a D-format instruction so it requires two operands. The
             * second one is the address to which the branch should be taken. We
             * have this readily available (in fact, it was provided by the
             * programmer) so let's just pass it on.
             */
            as_if.operands.push_back(each.operands.front());

            expand_if(baked, as_if, l2p);
        } else {
            /*
             * Real instructions should be pushed without any modification.
             */
            baked.push_back(std::move(each));
        }
    }

    return baked;
}

auto operand_or_throw(ast::Instruction const& insn, size_t const index)
    -> ast::Operand const&
{
    try {
        return insn.operands.at(index);
    } catch (std::out_of_range const&) {
        using viua::libs::errors::compile_time::Cause;
        using viua::libs::errors::compile_time::Error;
        throw Error{insn.opcode,
                    Cause::Too_few_operands,
                    ("operand " + std::to_string(index) + " not found")};
    }
}

auto emit_bytecode(std::vector<std::unique_ptr<ast::Node>> const& nodes,
                   std::vector<viua::arch::instruction_type>& text,
                   std::vector<uint8_t>& fn_table,
                   std::map<std::string, size_t> const& fn_offsets)
    -> std::map<std::string, uint64_t>
{
    auto const ops_count =
        1
        + std::accumulate(
            nodes.begin(),
            nodes.end(),
            size_t{0},
            [](size_t const acc,
               std::unique_ptr<ast::Node> const& each) -> size_t {
                if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
                    return 0;
                }

                auto& fn = static_cast<ast::Fn_def&>(*each);
                return (acc + fn.instructions.size());
            });

    {
        text.reserve(ops_count);
        text.resize(ops_count);

        using viua::arch::instruction_type;
        using viua::arch::ops::N;
        using viua::arch::ops::OPCODE;
        text.at(0) = N{static_cast<instruction_type>(OPCODE::HALT)}.encode();
    }

    auto fn_addresses = std::map<std::string, uint64_t>{};
    auto ip           = (text.data() + 1);
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& fn = static_cast<ast::Fn_def&>(*each);
        {
            /*
             * Save the function's address (offset into the .text section,
             * really) in the functions table. This is needed not only for
             * debugging, but also because the functions' addresses are resolved
             * dynamically for call and similar instructions. Why dynamically?
             *
             * Because there is a strong distinction between calls to bytecode
             * and foreign functions. At compile time, we don't yet know,
             * though, which function is foreign and which is bytecode.
             */
            auto const fn_addr =
                (ip - &text[0]) * sizeof(viua::arch::instruction_type);
            fn_addresses[fn.name.text] = fn_addr;
            patch_fn_address(fn_table, fn_offsets.at(fn.name.text), fn_addr);
        }

        for (auto const& insn : fn.instructions) {
            using viua::arch::opcode_type;
            using viua::arch::ops::FORMAT;
            using viua::arch::ops::FORMAT_MASK;

            auto opcode = opcode_type{};
            try {
                opcode = insn.parse_opcode();
            } catch (std::invalid_argument const&) {
                auto const e = insn.opcode;

                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;
                throw Error{e, Cause::Unknown_opcode, e.text};
            }
            auto format = static_cast<FORMAT>(opcode & FORMAT_MASK);
            switch (format) {
            case FORMAT::N:
                *ip++ = static_cast<uint64_t>(opcode);
                break;
            case FORMAT::T:
                *ip++ =
                    viua::arch::ops::T{opcode,
                                       operand_or_throw(insn, 0).make_access(),
                                       operand_or_throw(insn, 1).make_access(),
                                       operand_or_throw(insn, 2).make_access()}
                        .encode();
                break;
            case FORMAT::D:
                *ip++ =
                    viua::arch::ops::D{opcode,
                                       operand_or_throw(insn, 0).make_access(),
                                       operand_or_throw(insn, 1).make_access()}
                        .encode();
                break;
            case FORMAT::S:
                *ip++ =
                    viua::arch::ops::S{opcode,
                                       operand_or_throw(insn, 0).make_access()}
                        .encode();
                break;
            case FORMAT::F:
                break;  // FIXME
            case FORMAT::E:
                *ip++ =
                    viua::arch::ops::E{
                        opcode,
                        operand_or_throw(insn, 0).make_access(),
                        std::stoull(
                            operand_or_throw(insn, 1).ingredients.front().text)}
                        .encode();
                break;
            case FORMAT::R:
            {
                auto const imm = insn.operands.back().ingredients.front();
                auto const is_unsigned = (static_cast<opcode_type>(opcode)
                                          & viua::arch::ops::UNSIGNED);
                if (is_unsigned and imm.text.at(0) == '-'
                    and (imm.text != "-1" and imm.text != "-1u")) {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;
                    throw Error{imm,
                                Cause::Value_out_of_range,
                                "signed integer used for unsigned immediate"}
                        .note("the only signed value allowed in this context "
                              "is -1, and\n"
                              "it is used a symbol for maximum unsigned "
                              "immediate value");
                }
                if ((not is_unsigned) and imm.text.back() == 'u') {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;
                    throw Error{imm,
                                Cause::Value_out_of_range,
                                "unsigned integer used for signed immediate"};
                }
                try {
                    *ip++ =
                        viua::arch::ops::R{
                            opcode,
                            insn.operands.at(0).make_access(),
                            insn.operands.at(1).make_access(),
                            (is_unsigned
                                 ? static_cast<uint32_t>(std::stoul(imm.text))
                                 : static_cast<uint32_t>(std::stoi(imm.text)))}
                            .encode();
                } catch (std::invalid_argument const&) {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;
                    // FIXME make the error more precise, maybe encapsulate
                    // just the immediate operand conversion
                    throw Error{imm,
                                Cause::Invalid_operand,
                                "expected integer as immediate operand"};
                }
                break;
            }
            }
        }
    }

    return fn_addresses;
}

auto view_line_of(std::string_view sv, viua::libs::lexer::Location loc)
    -> std::string_view
{
    /*
     * We want to get a view of a single line out of a view of the entire source
     * code. This task can be achieved by finding the newlines before and after
     * the offset at which out location is located, and cutting the portion of
     * the string inside these two bounds.
     */
    {
        /*
         * The ending newline is easy - just find the next occurence after the
         * location we are interested in. If no can be found then we have
         * nothing to do because the line we want to view is the last line of
         * the source code.
         */
        auto line_end = size_t{0};
        line_end      = sv.find('\n', loc.offset);

        if (line_end != std::string::npos) {
            sv.remove_suffix(sv.size() - line_end);
        }
    }
    {
        /*
         * The starting newline is tricky. We need to find it, but if we do not
         * then it means that the line we want to view is the very fist line of
         * the source code...
         */
        auto line_begin = size_t{0};
        line_begin      = sv.rfind('\n', (loc.offset ? (loc.offset - 1) : 0));

        /*
         * ...and we have to treat it a little bit different. For any other line
         * we increase the beginning offset by 1 (to skip the actual newline
         * character), and for the first one we substitute the npos for 0 (to
         * start from the beginning).
         */
        line_begin = (line_begin == std::string::npos) ? 0 : (line_begin + 1);

        sv.remove_prefix(line_begin);
    }

    return sv;
}

auto view_line_before(std::string_view sv, viua::libs::lexer::Location loc)
    -> std::string_view
{
    auto line_end   = size_t{0};
    auto line_begin = size_t{0};

    line_end = sv.rfind('\n', (loc.offset ? (loc.offset - 1) : 0));
    if (line_end != std::string::npos) {
        sv.remove_suffix(sv.size() - line_end);
    }

    line_begin = sv.rfind('\n');
    if (line_begin != std::string::npos) {
        sv.remove_prefix(line_begin + 1);
    }

    return sv;
}

auto view_line_after(std::string_view sv, viua::libs::lexer::Location loc)
    -> std::string_view
{
    auto line_end   = size_t{0};
    auto line_begin = size_t{0};

    line_begin = sv.find('\n', loc.offset);
    if (line_begin != std::string::npos) {
        sv.remove_prefix(line_begin + 1);
    }

    line_end = sv.find('\n');
    if (line_end != std::string::npos) {
        sv.remove_suffix(sv.size() - line_end);
    }

    return sv;
}

auto cook_spans(
    std::vector<viua::libs::errors::compile_time::Error::span_type> raw)
    -> std::vector<std::tuple<bool, size_t, size_t>>
{
    /*
     * In case of synthesized lexemes, there may be several of them that have
     * the same offset. Naively including all of them in spans produces... weird
     * results when the output is formatted for an error report.
     *
     * Let's only use the first span at a given offset.
     */
    auto seen_offsets = std::set<size_t>{};

    auto cooked = std::vector<std::tuple<bool, size_t, size_t>>{};
    if (std::get<1>(raw.front()) != 0) {
        cooked.emplace_back(false, 0, raw.front().first);
        seen_offsets.insert(0);
    }
    for (auto const& each : raw) {
        auto const& [hl, offset, size] = cooked.back();
        if (auto const want = (offset + size); want < each.first) {
            if (not seen_offsets.contains(want)) {
                cooked.emplace_back(false, want, (each.first - want));
                seen_offsets.insert(want);
            }
        }
        if (not seen_offsets.contains(each.first)) {
            cooked.emplace_back(true, each.first, each.second);
            seen_offsets.insert(each.first);
        }
    }

    return cooked;
}
auto display_error_and_exit
    [[noreturn]] (std::filesystem::path source_path,
                  std::string_view source_text,
                  viua::libs::errors::compile_time::Error const& e) -> void
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    constexpr auto SEPARATOR_SOURCE = std::string_view{" | "};
    constexpr auto SEPARATOR_ASIDE  = std::string_view{" . "};
    constexpr auto ERROR_MARKER     = std::string_view{" => "};
    auto const LINE_NO_WIDTH =
        std::to_string(std::max(e.line(), e.line() + 1)).size();

    /*
     * The separator to put some space between the command and the error
     * report.
     */
    std::cerr << std::string(ERROR_MARKER.size(), ' ')
              << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR_SOURCE << "\n";

    if (e.line()) {
        std::cerr << std::string(ERROR_MARKER.size(), ' ')
                  << std::setw(LINE_NO_WIDTH) << e.line() << SEPARATOR_SOURCE
                  << view_line_before(source_text, e.location()) << "\n";
    }

    auto source_line    = std::ostringstream{};
    auto highlight_line = std::ostringstream{};

    if constexpr (false) {
        auto line = view_line_of(source_text, e.location());

        source_line << esc(2, COLOR_FG_RED) << ERROR_MARKER
                    << std::setw(LINE_NO_WIDTH) << (e.line() + 1)
                    << esc(2, COLOR_FG_WHITE) << SEPARATOR_SOURCE;
        highlight_line << std::string(ERROR_MARKER.size(), ' ')
                       << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR_SOURCE;

        source_line << std::string_view{line.data(), e.character()};
        highlight_line << std::string(e.character(), ' ');
        line.remove_prefix(e.character());

        /*
         * This if is required because of TERMINATOR tokens in unexpected
         * places. In case a TERMINATOR token is the cause of the error it
         * will not appear in line. If we attempted to shift line's head, it
         * would be removing a prefix from an empty std::string_view which
         * is undefined behaviour.
         *
         * I think the "bad TERMINATOR" is the only situation when this is
         * important.
         *
         * When it happens, we just don't print the terminator (which is a
         * newline), because a newline character will be added anyway.
         */
        if (not line.empty()) {
            source_line << esc(2, COLOR_FG_RED_1) << e.main().text
                        << esc(2, ATTR_RESET);
            line.remove_prefix(e.main().text.size());
        }
        highlight_line << esc(2, COLOR_FG_RED) << '^';
        highlight_line << esc(2, COLOR_FG_ORANGE_RED_1)
                       << std::string((e.main().text.size() - 1), '~')
                       << esc(2, ATTR_RESET);

        source_line << esc(2, COLOR_FG_WHITE) << line << esc(2, ATTR_RESET);
    }

    {
        auto line = view_line_of(source_text, e.location());

        source_line << esc(2, COLOR_FG_RED) << ERROR_MARKER
                    << std::setw(LINE_NO_WIDTH) << (e.line() + 1)
                    << esc(2, COLOR_FG_WHITE) << SEPARATOR_SOURCE;
        highlight_line << std::string(ERROR_MARKER.size(), ' ')
                       << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR_SOURCE;

        auto const spans = cook_spans(e.spans());
        for (auto const& each : spans) {
            auto const& [hl, offset, size] = each;
            if (hl and offset == e.character()) {
                source_line << esc(2, COLOR_FG_RED_1);
                highlight_line << esc(2, COLOR_FG_RED_1) << '^'
                               << esc(2, COLOR_FG_RED)
                               << std::string(size - 1, '~');
            } else if (hl) {
                source_line << esc(2, COLOR_FG_RED);
                highlight_line << esc(2, COLOR_FG_RED)
                               << std::string(size, '~');
            } else {
                source_line << esc(2, COLOR_FG_WHITE);
                highlight_line << std::string(size, ' ');
            }
            source_line << line.substr(offset, size);
        }
        source_line << esc(2, COLOR_FG_WHITE)
                    << line.substr(std::get<1>(spans.back())
                                   + std::get<2>(spans.back()));

        source_line << esc(2, ATTR_RESET);
        highlight_line << esc(2, ATTR_RESET);
    }

    std::cerr << source_line.str() << "\n";
    std::cerr << highlight_line.str() << "\n";

    if (not e.aside().empty()) {
        std::cerr << std::string(ERROR_MARKER.size(), ' ')
                  << std::string(LINE_NO_WIDTH, ' ') << esc(2, COLOR_FG_CYAN)
                  << SEPARATOR_ASIDE << std::string(e.character(), ' ') << '|'
                  << esc(2, ATTR_RESET) << "\n";
        std::cerr << std::string(ERROR_MARKER.size(), ' ')
                  << std::string(LINE_NO_WIDTH, ' ') << esc(2, COLOR_FG_CYAN)
                  << SEPARATOR_ASIDE << std::string(e.character(), ' ')
                  << e.aside() << esc(2, ATTR_RESET) << "\n";
        std::cerr << esc(2, COLOR_FG_CYAN)
                  << std::string(ERROR_MARKER.size(), ' ')
                  << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR_ASIDE
                  << esc(2, ATTR_RESET) << "\n";
    }

    {
        std::cerr << std::string(ERROR_MARKER.size(), ' ')
                  << std::setw(LINE_NO_WIDTH) << (e.line() + 2)
                  << SEPARATOR_SOURCE
                  << view_line_after(source_text, e.location()) << "\n";
    }

    /*
     * The separator to put some space between the source code dump,
     * highlight, etc and the error message.
     */
    std::cerr << std::string(ERROR_MARKER.size(), ' ')
              << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR_SOURCE << "\n";

    std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native() << esc(2, ATTR_RESET)
              << ':' << esc(2, COLOR_FG_WHITE) << (e.line() + 1)
              << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
              << (e.character() + 1) << esc(2, ATTR_RESET) << ": "
              << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET) << ": "
              << e.str() << "\n";

    for (auto const& each : e.notes()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native() << esc(2, ATTR_RESET)
                  << ':' << esc(2, COLOR_FG_WHITE) << (e.line() + 1)
                  << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
                  << (e.character() + 1) << esc(2, ATTR_RESET) << ": "
                  << esc(2, COLOR_FG_CYAN) << "note" << esc(2, ATTR_RESET)
                  << ": ";
        if (each.find('\n') == std::string::npos) {
            std::cerr << each << "\n";
        } else {
            auto const prefix_length =
                source_path.string().size()
                + std::to_string(e.line() + 1).size()
                + std::to_string(e.character() + 1).size() + 6  // for ": note"
                + 2  // for ":" after source path and line number
                ;

            auto sv = std::string_view{each};
            std::cerr << sv.substr(0, sv.find('\n')) << '\n';

            do {
                sv.remove_prefix(sv.find('\n') + 1);
                std::cerr << std::string(prefix_length, ' ') << "| "
                          << sv.substr(0, sv.find('\n')) << '\n';
            } while (sv.find('\n') != std::string::npos);
        }
    }

    exit(1);
}

auto display_error_in_function(std::filesystem::path const source_path,
                               viua::libs::errors::compile_time::Error const& e,
                               std::string_view const fn_name) -> void
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native() << esc(2, ATTR_RESET)
              << ':' << esc(2, COLOR_FG_WHITE) << (e.line() + 1)
              << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
              << (e.character() + 1) << esc(2, ATTR_RESET) << ": "
              << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
              << ": in function " << esc(2, COLOR_FG_WHITE) << fn_name
              << esc(2, ATTR_RESET) << ":\n";
}
}  // namespace

namespace stage {
using Lexemes   = std::vector<viua::libs::lexer::Lexeme>;
using AST_nodes = std::vector<std::unique_ptr<ast::Node>>;

auto lexical_analysis(std::filesystem::path const source_path,
                      std::string_view const source_text) -> Lexemes
{
    try {
        return viua::libs::lexer::lex(source_text);
    } catch (viua::libs::lexer::Location const& location) {
        using viua::support::tty::ATTR_RESET;
        using viua::support::tty::COLOR_FG_ORANGE_RED_1;
        using viua::support::tty::COLOR_FG_RED;
        using viua::support::tty::COLOR_FG_RED_1;
        using viua::support::tty::COLOR_FG_WHITE;
        using viua::support::tty::send_escape_seq;
        constexpr auto esc = send_escape_seq;

        auto const SEPARATOR         = std::string{" |  "};
        constexpr auto LINE_NO_WIDTH = size_t{5};

        auto source_line    = std::ostringstream{};
        auto highlight_line = std::ostringstream{};

        std::cerr << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR << "\n";

        struct {
            std::string text;
        } e;
        e.text = std::string(1, source_text[location.offset]);

        {
            auto line = view_line_of(source_text, location);

            source_line << esc(2, COLOR_FG_RED) << std::setw(LINE_NO_WIDTH)
                        << (location.line + 1) << esc(2, ATTR_RESET)
                        << SEPARATOR;
            highlight_line << std::string(LINE_NO_WIDTH, ' ') << SEPARATOR;

            source_line << std::string_view{line.data(), location.character};
            highlight_line << std::string(location.character, ' ');
            line.remove_prefix(location.character);

            /*
             * This if is required because of TERMINATOR tokens in unexpected
             * places. In case a TERMINATOR token is the cause of the error it
             * will not appear in line. If we attempted to shift line's head, it
             * would be removing a prefix from an empty std::string_view which
             * is undefined behaviour.
             *
             * I think the "bad TERMINATOR" is the only situation when this is
             * important.
             *
             * When it happens, we just don't print the terminator (which is a
             * newline), because a newline character will be added anyway.
             */
            if (not line.empty()) {
                source_line << esc(2, COLOR_FG_RED_1) << e.text
                            << esc(2, ATTR_RESET);
                line.remove_prefix(e.text.size());
            }
            highlight_line << esc(2, COLOR_FG_RED) << '^';
            highlight_line << esc(2, COLOR_FG_ORANGE_RED_1)
                           << std::string((e.text.size() - 1), '~');

            source_line << line;
        }

        std::cerr << source_line.str() << "\n";
        std::cerr << highlight_line.str() << "\n";

        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native() << esc(2, ATTR_RESET)
                  << ':' << esc(2, COLOR_FG_WHITE) << (location.line + 1)
                  << esc(2, ATTR_RESET) << ':' << esc(2, COLOR_FG_WHITE)
                  << (location.character + 1) << esc(2, ATTR_RESET) << ": "
                  << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": "
                  << "no token match at character "
                  << viua::support::string::CORNER_QUOTE_LL
                  << esc(2, COLOR_FG_WHITE) << e.text << esc(2, ATTR_RESET)
                  << viua::support::string::CORNER_QUOTE_UR << "\n";

        exit(1);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        display_error_and_exit(source_path, source_text, e);
    }
}

auto syntactical_analysis(std::filesystem::path const source_path,
                          std::string_view const source_text,
                          Lexemes const& lexemes) -> AST_nodes
{
    try {
        return parse(lexemes);
    } catch (viua::libs::lexer::Lexeme const& e) {
        display_error_and_exit(source_path, source_text, e);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        display_error_and_exit(source_path, source_text, e);
    }
}

auto load_value_labels(std::filesystem::path const source_path,
                       std::string_view const source_text,
                       AST_nodes const& nodes,
                       std::vector<uint8_t>& strings_table,
                       std::map<std::string, size_t>& var_offsets) -> void
{
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Label_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& ct = static_cast<ast::Label_def&>(*each);
        if (ct.type == "string") {
            auto s = std::string{};
            for (auto i = size_t{0}; i < ct.value.size(); ++i) {
                auto& each = ct.value.at(i);

                using enum viua::libs::lexer::TOKEN;
                if (each.token == LITERAL_STRING) {
                    auto tmp = each.text;
                    tmp      = tmp.substr(1, tmp.size() - 2);
                    tmp      = viua::support::string::unescape(tmp);
                    s += tmp;
                } else if (each.token == RA_PTR_DEREF) {
                    auto& next = ct.value.at(++i);
                    if (next.token != LITERAL_INTEGER) {
                        using viua::libs::errors::compile_time::Cause;
                        using viua::libs::errors::compile_time::Error;

                        auto const e = Error{each,
                                             Cause::Invalid_operand,
                                             "cannot multiply string constant "
                                             "by non-integer"}
                                           .add(next)
                                           .add(ct.value.at(i - 2))
                                           .aside("right-hand side must be an "
                                                  "positive integer");
                        display_error_and_exit(source_path, source_text, e);
                    }

                    auto x = ston<size_t>(next.text);
                    auto o = std::ostringstream{};
                    for (auto i = size_t{0}; i < x; ++i) {
                        o << s;
                    }
                    s = o.str();
                }
            }

            var_offsets[ct.name.text] = save_string(strings_table, s);
        } else if (ct.type == "atom") {
            auto const s = ct.value.front().text;
            var_offsets[ct.name.text] = save_string(strings_table, s);
        }
    }
}

auto load_function_labels(AST_nodes const& nodes,
                          std::vector<uint8_t>& fn_table,
                          std::map<std::string, size_t>& fn_offsets) -> void
{
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& fn = static_cast<ast::Fn_def&>(*each);
        fn_offsets.emplace(fn.name.text,
                           save_fn_address(fn_table, fn.name.text));
    }
}

auto cook_long_immediates(std::filesystem::path const source_path,
                          std::string_view const source_text,
                          AST_nodes const& nodes,
                          std::vector<uint8_t>& strings_table,
                          std::map<std::string, size_t>& var_offsets) -> void
{
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& fn = static_cast<ast::Fn_def&>(*each);

        auto cooked = std::vector<ast::Instruction>{};
        for (auto& insn : fn.instructions) {
            if (insn.opcode == "atom" or insn.opcode == "g.atom") {
                auto const lx = insn.operands.back().ingredients.front();
                auto s        = lx.text;
                auto saved_at = size_t{0};
                if (lx.token == viua::libs::lexer::TOKEN::LITERAL_STRING) {
                    s = s.substr(1, s.size() - 2);
                    s = viua::support::string::unescape(s);
                    saved_at = save_string(strings_table, s);
                } else if (lx.token == viua::libs::lexer::TOKEN::LITERAL_ATOM) {
                    auto s   = lx.text;
                    saved_at = save_string(strings_table, s);
                } else if (lx.token == viua::libs::lexer::TOKEN::AT) {
                    auto const label = insn.operands.back().ingredients.back();
                    try {
                        saved_at = var_offsets.at(label.text);
                    } catch (std::out_of_range const&) {
                        using viua::libs::errors::compile_time::Cause;
                        using viua::libs::errors::compile_time::Error;

                        auto e = Error{label, Cause::Unknown_label, label.text};
                        e.add(lx);

                        using viua::support::string::levenshtein_filter;
                        auto misspell_candidates =
                            levenshtein_filter(label.text, var_offsets);
                        if (not misspell_candidates.empty()) {
                            using viua::support::string::levenshtein_best;
                            auto best_candidate =
                                levenshtein_best(label.text,
                                                 misspell_candidates,
                                                 (label.text.size() / 2));
                            if (best_candidate.second != label.text) {
                                did_you_mean(e, best_candidate.second);
                            }
                        }

                        display_error_in_function(source_path, e, fn.name.text);
                        display_error_and_exit(source_path, source_text, e);
                    }
                } else {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;

                    auto e = Error{lx, Cause::Invalid_operand}.aside(
                        "expected string literal, atom literal, or a label reference");

                    if (lx.token == viua::libs::lexer::TOKEN::LITERAL_ATOM) {
                        did_you_mean(e, '@' + lx.text);
                    }

                    display_error_in_function(source_path, e, fn.name.text);
                    display_error_and_exit(source_path, source_text, e);
                }

                auto synth           = ast::Instruction{};
                synth.opcode         = insn.opcode;
                synth.opcode.text    = "g.li";
                synth.physical_index = insn.physical_index;

                synth.operands.push_back(insn.operands.front());
                synth.operands.push_back(insn.operands.back());
                synth.operands.back().ingredients.front().text =
                    std::to_string(saved_at) + 'u';

                cooked.push_back(synth);

                insn.operands.pop_back();
                cooked.push_back(std::move(insn));
            } else if (insn.opcode == "string" or insn.opcode == "g.string") {
                auto const lx = insn.operands.back().ingredients.front();
                auto saved_at = size_t{0};
                if (lx.token == viua::libs::lexer::TOKEN::LITERAL_STRING) {
                    auto s   = lx.text;
                    s        = s.substr(1, s.size() - 2);
                    s        = viua::support::string::unescape(s);
                    saved_at = save_string(strings_table, s);
                } else if (lx.token == viua::libs::lexer::TOKEN::AT) {
                    auto const label = insn.operands.back().ingredients.back();
                    try {
                        saved_at = var_offsets.at(label.text);
                    } catch (std::out_of_range const&) {
                        using viua::libs::errors::compile_time::Cause;
                        using viua::libs::errors::compile_time::Error;

                        auto e = Error{label, Cause::Unknown_label, label.text};
                        e.add(lx);

                        using viua::support::string::levenshtein_filter;
                        auto misspell_candidates =
                            levenshtein_filter(label.text, var_offsets);
                        if (not misspell_candidates.empty()) {
                            using viua::support::string::levenshtein_best;
                            auto best_candidate =
                                levenshtein_best(label.text,
                                                 misspell_candidates,
                                                 (label.text.size() / 2));
                            if (best_candidate.second != label.text) {
                                did_you_mean(e, best_candidate.second);
                            }
                        }

                        display_error_in_function(source_path, e, fn.name.text);
                        display_error_and_exit(source_path, source_text, e);
                    }
                } else {
                    using viua::libs::errors::compile_time::Cause;
                    using viua::libs::errors::compile_time::Error;

                    auto e = Error{lx, Cause::Invalid_operand}.aside(
                        "expected string literal, or a label reference");

                    if (lx.token == viua::libs::lexer::TOKEN::LITERAL_ATOM) {
                        did_you_mean(e, '@' + lx.text);
                    }

                    display_error_in_function(source_path, e, fn.name.text);
                    display_error_and_exit(source_path, source_text, e);
                }

                auto synth           = ast::Instruction{};
                synth.opcode         = insn.opcode;
                synth.opcode.text    = "g.li";
                synth.physical_index = insn.physical_index;

                synth.operands.push_back(insn.operands.front());
                synth.operands.push_back(insn.operands.back());
                synth.operands.back().ingredients.front().text =
                    std::to_string(saved_at) + 'u';

                cooked.push_back(synth);

                insn.operands.pop_back();
                cooked.push_back(std::move(insn));
            } else if (insn.opcode == "float" or insn.opcode == "g.float") {
                constexpr auto SIZE_OF_SINGLE_PRECISION_FLOAT = size_t{4};
                auto f =
                    std::stof(insn.operands.back().ingredients.front().text);
                auto s = std::string(SIZE_OF_SINGLE_PRECISION_FLOAT, '\0');
                memcpy(s.data(), &f, SIZE_OF_SINGLE_PRECISION_FLOAT);
                auto const saved_at = save_string(strings_table, s);

                auto synth           = ast::Instruction{};
                synth.opcode         = insn.opcode;
                synth.opcode.text    = "g.li";
                synth.physical_index = insn.physical_index;

                synth.operands.push_back(insn.operands.front());
                synth.operands.push_back(insn.operands.back());
                synth.operands.back().ingredients.front().text =
                    std::to_string(saved_at) + 'u';

                cooked.push_back(synth);

                insn.operands.pop_back();
                cooked.push_back(std::move(insn));
            } else if (insn.opcode == "double" or insn.opcode == "g.double") {
                constexpr auto SIZE_OF_DOUBLE_PRECISION_FLOAT = size_t{8};
                auto f =
                    std::stod(insn.operands.back().ingredients.front().text);
                auto s = std::string(SIZE_OF_DOUBLE_PRECISION_FLOAT, '\0');
                memcpy(s.data(), &f, SIZE_OF_DOUBLE_PRECISION_FLOAT);
                auto const saved_at = save_string(strings_table, s);

                auto synth           = ast::Instruction{};
                synth.opcode         = insn.opcode;
                synth.opcode.text    = "g.li";
                synth.physical_index = insn.physical_index;

                synth.operands.push_back(insn.operands.front());
                synth.operands.push_back(insn.operands.back());
                synth.operands.back().ingredients.front().text =
                    std::to_string(saved_at) + 'u';

                cooked.push_back(synth);

                insn.operands.pop_back();
                cooked.push_back(std::move(insn));
            } else {
                cooked.push_back(std::move(insn));
            }
        }
        fn.instructions = std::move(cooked);
    }
}

auto cook_pseudoinstructions(std::filesystem::path const source_path,
                             std::string_view const source_text,
                             AST_nodes& nodes,
                             std::map<std::string, size_t>& fn_offsets) -> void
{
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& fn                 = static_cast<ast::Fn_def&>(*each);
        auto const raw_ops_count = fn.instructions.size();
        try {
            fn.instructions = ::expand_pseudoinstructions(
                std::move(fn.instructions), fn_offsets);
        } catch (viua::libs::errors::compile_time::Error const& e) {
            display_error_in_function(source_path, e, fn.name.text);
            display_error_and_exit(source_path, source_text, e);
        }

        if constexpr (DEBUG_EXPANSION) {
            std::cerr << "FN " << fn.to_string() << " with " << raw_ops_count
                      << " raw, " << fn.instructions.size() << " baked op(s)\n";
            auto physical_index = size_t{0};
            for (auto const& op : fn.instructions) {
                std::cerr << "  " << std::setw(4) << std::setfill('0')
                          << std::hex << physical_index++ << " " << std::setw(4)
                          << std::setfill('0') << std::hex << op.physical_index
                          << "  " << op.to_string() << "\n";
            }
        }
    }
}

auto find_entry_point(std::filesystem::path const source_path,
                      std::string_view const source_text,
                      bool const as_executable,
                      AST_nodes const& nodes)
    -> std::optional<viua::libs::lexer::Lexeme>
{
    auto entry_point_fn = std::optional<viua::libs::lexer::Lexeme>{};
    for (auto const& each : nodes) {
        if (each->has_attr("entry_point")) {
            if (entry_point_fn.has_value()) {
                using viua::libs::errors::compile_time::Cause;
                using viua::libs::errors::compile_time::Error;

                auto const dup = static_cast<ast::Fn_def&>(*each);
                auto const e =
                    Error{
                        dup.name, Cause::Duplicated_entry_point, dup.name.text}
                        .add(dup.attr("entry_point").value())
                        .note("first entry point was: " + entry_point_fn->text);
                display_error_and_exit(source_path, source_text, e);
            }
            entry_point_fn = static_cast<ast::Fn_def&>(*each).name;
        }
    }
    if (as_executable and not entry_point_fn.has_value()) {
        using viua::support::tty::ATTR_RESET;
        using viua::support::tty::COLOR_FG_CYAN;
        using viua::support::tty::COLOR_FG_RED;
        using viua::support::tty::COLOR_FG_WHITE;
        using viua::support::tty::send_escape_seq;
        constexpr auto esc = send_escape_seq;

        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native() << esc(2, ATTR_RESET)
                  << ": " << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET) << ": "
                  << "no entry point function defined\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native() << esc(2, ATTR_RESET)
                  << ": " << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": "
                  << "the entry function should have the [[entry_point]] "
                     "attribute\n";
        exit(1);
    }
    return entry_point_fn;
}

using Text         = std::vector<viua::arch::instruction_type>;
using Fn_addresses = std::map<std::string, uint64_t>;
auto emit_bytecode(std::filesystem::path const source_path,
                   std::string_view const source_text,
                   AST_nodes const& nodes,
                   std::vector<uint8_t>& fn_table,
                   std::map<std::string, size_t> const& fn_offsets)
    -> std::pair<Text, Fn_addresses>
{
    /*
     * Calculate function spans in source code for error reporting. This way an
     * error offset can be matched to a function without the error having to
     * carry the function name.
     */
    auto fn_spans =
        std::vector<std::pair<std::string, std::pair<size_t, size_t>>>{};
    for (auto const& each : nodes) {
        if (dynamic_cast<ast::Fn_def*>(each.get()) == nullptr) {
            continue;
        }

        auto& fn = static_cast<ast::Fn_def&>(*each);
        fn_spans.emplace_back(
            fn.name.text,
            std::pair<size_t, size_t>{fn.start.location.offset,
                                      fn.end.location.offset});
    }

    auto text         = std::vector<viua::arch::instruction_type>{};
    auto fn_addresses = std::map<std::string, uint64_t>{};
    try {
        fn_addresses = ::emit_bytecode(nodes, text, fn_table, fn_offsets);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        auto fn_name = std::optional<std::string>{};

        for (auto const& [name, offs] : fn_spans) {
            auto const [low, high] = offs;
            auto const off         = e.location().offset;
            if ((off >= low) and (off <= high)) {
                fn_name = name;
            }
        }

        if (fn_name.has_value()) {
            display_error_in_function(source_path, e, *fn_name);
        }
        display_error_and_exit(source_path, source_text, e);
    }

    return {text, fn_addresses};
}

auto emit_elf(std::filesystem::path const output_path,
              bool const as_executable,
              std::optional<uint64_t> const entry_point_fn,
              Text const& text,
              std::vector<uint8_t> const& strings_table,
              std::vector<uint8_t> const& fn_table) -> void
{
    auto const a_out = open(output_path.c_str(),
                            O_CREAT | O_TRUNC | O_WRONLY,
                            S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (a_out == -1) {
        close(a_out);
        exit(1);
    }

    constexpr auto VIUA_MAGIC [[maybe_unused]] = "\x7fVIUA\x00\x00\x00";
    auto const VIUAVM_INTERP                   = std::string{"viua-vm"};

    {
        // see elf(5)
        Elf64_Ehdr elf_header{};
        elf_header.e_ident[EI_MAG0]       = '\x7f';
        elf_header.e_ident[EI_MAG1]       = 'E';
        elf_header.e_ident[EI_MAG2]       = 'L';
        elf_header.e_ident[EI_MAG3]       = 'F';
        elf_header.e_ident[EI_CLASS]      = ELFCLASS64;
        elf_header.e_ident[EI_DATA]       = ELFDATA2LSB;
        elf_header.e_ident[EI_VERSION]    = EV_CURRENT;
        elf_header.e_ident[EI_OSABI]      = ELFOSABI_STANDALONE;
        elf_header.e_ident[EI_ABIVERSION] = 0;
        elf_header.e_type                 = (as_executable ? ET_EXEC : ET_REL);
        elf_header.e_machine              = ET_NONE;
        elf_header.e_version              = elf_header.e_ident[EI_VERSION];
        elf_header.e_flags  = 0;  // processor-specific flags, should be 0
        elf_header.e_ehsize = sizeof(elf_header);

        auto shstr            = std::vector<char>{'\0'};
        auto save_shstr_entry = [&shstr](std::string_view const sv) -> size_t {
            auto const saved_at = shstr.size();
            std::copy(sv.begin(), sv.end(), std::back_inserter(shstr));
            shstr.push_back('\0');
            return saved_at;
        };

        using Header_pair = std::pair<std::optional<Elf64_Phdr>, Elf64_Shdr>;
        auto elf_headers  = std::vector<Header_pair>{};

        {
            /*
             * It is mandated by ELF that the first section header is void, and
             * must be all zeroes. It is reserved and used by ELF extensions.
             *
             * We do not extend ELF in any way, so this section is SHT_NULL for
             * Viua VM.
             */
            Elf64_Shdr void_section{};
            void_section.sh_type = SHT_NULL;

            elf_headers.push_back({std::nullopt, void_section});
        }
        {
            /*
             * The second section (and the first fragment) is the magic number
             * Viua uses to detect the if the binary *really* is something it
             * can handle, and on Linux by the binfmt.d(5) to enable running
             * Viua ELFs automatically.
             */
            Elf64_Phdr seg{};
            seg.p_type   = PT_NULL;
            seg.p_offset = 0;
            memcpy(&seg.p_offset, VIUA_MAGIC, 8);
            seg.p_filesz = 8;

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".viua.magic");
            sec.sh_type   = SHT_NOBITS;
            sec.sh_offset = sizeof(Elf64_Ehdr) + offsetof(Elf64_Phdr, p_offset);
            sec.sh_size   = 8;
            sec.sh_flags  = 0;

            elf_headers.push_back({seg, sec});
        }
        {
            /*
             * What follows is the interpreter. This is mostly useful to get
             * better reporting out of readelf(1) and file(1). It also serves as
             * a second thing to check for if the file *really* is a Viua
             * binary.
             */

            Elf64_Phdr seg{};
            seg.p_type   = PT_INTERP;
            seg.p_offset = 0;
            seg.p_filesz = VIUAVM_INTERP.size() + 1;
            seg.p_flags  = PF_R;

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".interp");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = VIUAVM_INTERP.size() + 1;
            sec.sh_flags  = 0;

            elf_headers.push_back({seg, sec});
        }
        {
            /*
             * The first segment and section pair that contains something users
             * of Viua can affect is the .text section ie, the executable
             * instructions representing user programs.
             */
            Elf64_Phdr seg{};
            seg.p_type   = PT_LOAD;
            seg.p_offset = 0;
            auto const sz =
                (text.size()
                 * sizeof(std::decay_t<decltype(text)>::value_type));
            seg.p_filesz = seg.p_memsz = sz;
            seg.p_flags                = PF_R | PF_X;
            seg.p_align                = sizeof(viua::arch::instruction_type);

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".text");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = seg.p_filesz;
            sec.sh_flags  = SHF_ALLOC | SHF_EXECINSTR;

            elf_headers.push_back({seg, sec});
        }
        {
            /*
             * Then, the .rodata section containing user data. Only constants
             * are allowed to be defined as data labels in Viua -- there are no
             * global variables.
             *
             * The "strings table" contains not only strings but also floats,
             * atoms, and any other piece of data that does not fit into a
             * single load instruction (with the exception of long integers
             * which are loaded using a sequence of raw instructions - this
             * allows loading addresses, which are then used to index strings
             * table).
             */
            Elf64_Phdr seg{};
            seg.p_type    = PT_LOAD;
            seg.p_offset  = 0;
            auto const sz = strings_table.size();
            seg.p_filesz = seg.p_memsz = sz;
            seg.p_flags                = PF_R;
            seg.p_align                = sizeof(viua::arch::instruction_type);

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".rodata");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = seg.p_filesz;
            sec.sh_flags  = SHF_ALLOC;

            elf_headers.push_back({seg, sec});
        }
        {
            /*
             * Last, but not least, comes another LOAD segment. This one is
             * mapped to a non-standard named section: .viua.fns It contains a
             * symbol table with function addresses.
             *
             * Function calls use this table to determine the address to which
             * they should transfer control - there are no direct calls.
             * Inefficient, but flexible.
             */
            Elf64_Phdr seg{};
            seg.p_type    = PT_LOAD;
            seg.p_offset  = 0;
            auto const sz = fn_table.size();
            seg.p_filesz = seg.p_memsz = sz;
            seg.p_flags                = PF_R;
            seg.p_align                = sizeof(viua::arch::instruction_type);

            Elf64_Shdr sec{};
            sec.sh_name = save_shstr_entry(".viua.fns");
            /*
             * This could be SHT_SYMTAB, but the SHT_SYMTAB type sections expect
             * a certain format of the symbol table which Viua does not use. So
             * let's just use SHT_PROGBITS because interpretation of
             * SHT_PROGBITS is up to the program.
             */
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = seg.p_filesz;
            sec.sh_flags  = SHF_ALLOC;

            elf_headers.push_back({seg, sec});
        }
        {
            /*
             * ACHTUNG! ATTENTION! UWAGA! POZOR! TÄHELEPANU!
             *
             * This section contains the strings table representing section
             * names. If any more sections are added they MUST APPEAR BEFORE
             * THIS SECTION. Otherwise the strings won't be available because
             * the size of the section will not be correct and will appear as
             * <corrupt> in readelf(1) output.
             */
            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".shstrtab");
            sec.sh_type   = SHT_STRTAB;
            sec.sh_offset = 0;
            sec.sh_size   = shstr.size();

            elf_headers.push_back({std::nullopt, sec});
        }

        auto elf_pheaders = std::count_if(
            elf_headers.begin(),
            elf_headers.end(),
            [](auto const& each) -> bool { return each.first.has_value(); });
        auto elf_sheaders = elf_headers.size();

        auto const elf_size = sizeof(Elf64_Ehdr)
                              + (elf_pheaders * sizeof(Elf64_Phdr))
                              + (elf_sheaders * sizeof(Elf64_Shdr));
        auto text_offset = std::optional<size_t>{};
        {
            auto offset_accumulator = size_t{0};
            for (auto& [segment, section] : elf_headers) {
                if (segment.has_value() and (segment->p_type != PT_NULL)) {
                    if (segment->p_type == PT_NULL) {
                        continue;
                    }

                    /*
                     * The thing that Viua VM mandates is that the main function
                     * (if it exists) MUST be put in the first executable
                     * segment. This can be elegantly achieved by blindly
                     * pushing the address of first such segment.
                     *
                     * The following construction using std::optional:
                     *
                     *      x = x.value_or(y)
                     *
                     * ensures that x will store the first assigned value
                     * without any checks. Why not use somethin more C-like? For
                     * example:
                     *
                     *      x = (x ? x : y)
                     *
                     * looks like it achieves the same without any fancy-shmancy
                     * types. Yeah, it only looks like it does so. If the first
                     * executable segment would happen to be at offset 0 then
                     * the C-style code fails, while the C++-style is correct.
                     * As an aside: this ie, C style being broken an C++ being
                     * correct is something surprisingly common. Or rather more
                     * functional style being correct... But I digress.
                     */
                    if (segment->p_flags == (PF_R | PF_X)) {
                        text_offset = text_offset.value_or(offset_accumulator);
                    }

                    segment->p_offset = (elf_size + offset_accumulator);
                }

                if (section.sh_type == SHT_NULL) {
                    continue;
                }
                if (section.sh_type == SHT_NOBITS) {
                    continue;
                }

                section.sh_offset = (elf_size + offset_accumulator);
                offset_accumulator += section.sh_size;
            }
        }

        elf_header.e_entry = entry_point_fn.has_value()
                                 ? (*text_offset + *entry_point_fn + elf_size)
                                 : 0;

        elf_header.e_phoff = sizeof(Elf64_Ehdr);
        ;
        elf_header.e_phentsize = sizeof(Elf64_Phdr);
        elf_header.e_phnum     = elf_pheaders;

        elf_header.e_shoff =
            elf_header.e_phoff + (elf_pheaders * sizeof(Elf64_Phdr));
        elf_header.e_shentsize = sizeof(Elf64_Shdr);
        elf_header.e_shnum     = elf_sheaders;
        elf_header.e_shstrndx  = elf_sheaders - 1;

        write(a_out, &elf_header, sizeof(elf_header));

        /*
         * Unfortunately, we have to have use two loops here because segment and
         * section headers cannot be interweaved. We could do some lseek(2)
         * tricks, but I don't think it's worth it. For-each loops are simple
         * and do not require any special bookkeeping to work correctly.
         */
        for (auto const& [segment, _] : elf_headers) {
            if (not segment) {
                continue;
            }
            write(a_out,
                  &*segment,
                  sizeof(std::remove_reference_t<decltype(*segment)>));
        }
        for (auto const& [_, section] : elf_headers) {
            write(a_out,
                  &section,
                  sizeof(std::remove_reference_t<decltype(section)>));
        }

        write(a_out, VIUAVM_INTERP.c_str(), VIUAVM_INTERP.size() + 1);

        write(a_out,
              text.data(),
              (text.size() * sizeof(std::decay_t<decltype(text)>::value_type)));
        write(a_out, strings_table.data(), strings_table.size());
        write(a_out, fn_table.data(), fn_table.size());

        write(a_out, shstr.data(), shstr.size());
    }

    close(a_out);
}
}  // namespace stage

auto main(int argc, char* argv[]) -> int
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    auto const args = std::vector<std::string>{(argv + 1), (argv + argc)};
    if (args.empty()) {
        std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                  << ": no file to assemble\n";
        return 1;
    }

    auto preferred_output_path = std::optional<std::filesystem::path>{};
    auto as_executable         = true;
    auto verbosity_level       = 0;
    auto show_version          = false;

    for (auto i = decltype(args)::size_type{}; i < args.size(); ++i) {
        auto const& each = args.at(i);
        if (each == "--") {
            // explicit separator of options and operands
            ++i;
            break;
        }
        /*
         * Tool-specific options.
         */
        else if (each == "-o") {
            preferred_output_path = std::filesystem::path{args.at(++i)};
        } else if (each == "-c") {
            as_executable = false;
        }
        /*
         * Common options.
         */
        else if (each == "-v" or each == "--verbose") {
            ++verbosity_level;
        } else if (each == "--version") {
            show_version = true;
        } else if (each.front() == '-') {
            // unknown option
        } else {
            // input files start here
            ++i;
            break;
        }
    }

    if (show_version) {
        if (verbosity_level) {
            std::cout << "Viua VM ";
        }
        std::cout << (verbosity_level ? VIUAVM_VERSION_FULL : VIUAVM_VERSION)
                  << "\n";
        return 0;
    }

    /*
     * If invoked *with* some arguments, find the path to the source file and
     * assemble it - converting assembly source code into binary. Produced
     * binary may be:
     *
     *  - executable (default): an ELF executable, suitable to be run by Viua VM
     *    kernel
     *  - linkable (with -c flag): an ELF relocatable object file, which should
     *    be linked with other object files to produce a final executable or
     *    shared object
     */
    auto const source_path = std::filesystem::path{args.back()};
    auto source_text       = std::string{};
    {
        auto const source_fd = open(source_path.c_str(), O_RDONLY);
        if (source_fd == -1) {
            using viua::support::tty::ATTR_RESET;
            using viua::support::tty::COLOR_FG_RED;
            using viua::support::tty::COLOR_FG_WHITE;
            using viua::support::tty::send_escape_seq;
            constexpr auto esc = send_escape_seq;

            auto const error_message = strerrordesc_np(errno);
            std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                      << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                      << "error" << esc(2, ATTR_RESET) << ": " << error_message
                      << "\n";
            return 1;
        }

        struct stat source_stat {
        };
        if (fstat(source_fd, &source_stat) == -1) {
            using viua::support::tty::ATTR_RESET;
            using viua::support::tty::COLOR_FG_RED;
            using viua::support::tty::COLOR_FG_WHITE;
            using viua::support::tty::send_escape_seq;
            constexpr auto esc = send_escape_seq;

            auto const error_message = strerrordesc_np(errno);
            std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                      << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                      << "error" << esc(2, ATTR_RESET) << ": " << error_message
                      << "\n";
            return 1;
        }
        if (source_stat.st_size == 0) {
            using viua::support::tty::ATTR_RESET;
            using viua::support::tty::COLOR_FG_RED;
            using viua::support::tty::COLOR_FG_WHITE;
            using viua::support::tty::send_escape_seq;
            constexpr auto esc = send_escape_seq;

            std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                      << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                      << "error" << esc(2, ATTR_RESET)
                      << ": empty source file\n";
            return 1;
        }

        source_text.resize(source_stat.st_size);
        read(source_fd, source_text.data(), source_text.size());
        close(source_fd);
    }

    auto const output_path = preferred_output_path.value_or(
        as_executable ? std::filesystem::path{"a.out"}
                      : [source_path]() -> std::filesystem::path {
            auto o = source_path;
            o.replace_extension("o");
            return o;
        }());

    /*
     * Lexical analysis (lexing).
     *
     * Split the loaded source code into a stream of lexemes for easier
     * processing later. The first point at which errors are detected eg, if
     * illegal characters are used, strings are unclosed, etc.
     */
    auto lexemes = stage::lexical_analysis(source_path, source_text);
    if constexpr (DEBUG_LEX) {
        std::cerr << lexemes.size() << " raw lexeme(s)\n";
        for (auto const& each : lexemes) {
            std::cerr << "  " << viua::libs::lexer::to_string(each.token) << ' '
                      << each.location.line << ':' << each.location.character
                      << '-' << (each.location.character + each.text.size() - 1)
                      << " +" << each.location.offset;

            using viua::libs::lexer::TOKEN;
            auto const printable = (each.token == TOKEN::LITERAL_STRING)
                                   or (each.token == TOKEN::LITERAL_INTEGER)
                                   or (each.token == TOKEN::LITERAL_FLOAT)
                                   or (each.token == TOKEN::LITERAL_ATOM)
                                   or (each.token == TOKEN::OPCODE);
            if (printable) {
                std::cerr << " " << each.text;
            }

            std::cerr << "\n";
        }
    }

    lexemes = ast::remove_noise(std::move(lexemes));
    if constexpr (DEBUG_LEX) {
        std::cerr << lexemes.size() << " cooked lexeme(s)\n";
        if constexpr (false) {
            for (auto const& each : lexemes) {
                std::cerr << "  " << viua::libs::lexer::to_string(each.token)
                          << ' ' << each.location.line << ':'
                          << each.location.character << '-'
                          << (each.location.character + each.text.size() - 1)
                          << " +" << each.location.offset;

                using viua::libs::lexer::TOKEN;
                auto const printable = (each.token == TOKEN::LITERAL_STRING)
                                       or (each.token == TOKEN::LITERAL_INTEGER)
                                       or (each.token == TOKEN::LITERAL_FLOAT)
                                       or (each.token == TOKEN::LITERAL_ATOM)
                                       or (each.token == TOKEN::OPCODE);
                if (printable) {
                    std::cerr << " " << each.text;
                }

                std::cerr << "\n";
            }
        }
    }

    /*
     * Syntactical analysis (parsing).
     *
     * Convert raw stream of lexemes into an abstract syntax tree structure that
     * groups lexemes representing a single entity (eg, a register access
     * specification) into a single object, and represents the relationships
     * between such objects.
     */
    auto nodes = stage::syntactical_analysis(source_path, source_text, lexemes);

    /*
     * String table preparation.
     *
     * Replace string, atom, float, and double literals in operands with offsets
     * into the string table. We want all instructions to fit into 64 bits, so
     * having variable-size operands is not an option.
     *
     * Don't move the strings table preparation after the pseudoinstruction
     * expansion stage. li pseudoinstructions are emitted during strings table
     * preparation so they need to be expanded.
     */
    auto strings_table = std::vector<uint8_t>{};
    auto var_offsets   = std::map<std::string, size_t>{};
    auto fn_table      = std::vector<uint8_t>{};
    auto fn_offsets    = std::map<std::string, size_t>{};

    stage::load_value_labels(
        source_path, source_text, nodes, strings_table, var_offsets);
    stage::load_function_labels(nodes, fn_table, fn_offsets);
    stage::cook_long_immediates(
        source_path, source_text, nodes, strings_table, var_offsets);

    /*
     * Pseudoinstruction- and macro-expansion.
     *
     * Replace pseudoinstructions (eg, li) with sequences of real instructions
     * that will have the same effect. Ditto for macros.
     */
    stage::cook_pseudoinstructions(source_path, source_text, nodes, fn_offsets);

    /*
     * Detect entry point function.
     *
     * We're not handling relocatable files (shared libs, etc) yet so it makes
     * sense to enforce entry function presence in all cases. Once the
     * relocatables and separate compilation is supported again, this should be
     * hidden behind a flag.
     */
    auto const entry_point_fn =
        stage::find_entry_point(source_path, source_text, as_executable, nodes);

    /*
     * Bytecode emission.
     *
     * This stage is also responsible for preparing the function table. It is a
     * table mapping function names to the offsets inside the .text section, at
     * which their entry points reside.
     */
    auto [text, fn_addresses] = stage::emit_bytecode(
        source_path, source_text, nodes, fn_table, fn_offsets);

    /*
     * ELF emission.
     */
    stage::emit_elf(
        output_path,
        as_executable,
        (entry_point_fn.has_value()
             ? std::optional{fn_addresses[entry_point_fn.value().text]}
             : std::nullopt),
        text,
        strings_table,
        fn_table);

    return 0;
}
