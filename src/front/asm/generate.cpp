/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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
#include <cstdint>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <viua/assembler/frontend/parser.h>
#include <viua/assembler/util/pretty_printer.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/tokenizer.h>
#include <viua/cg/tools.h>
#include <viua/front/asm.h>
#include <viua/loader.h>
#include <viua/machine.h>
#include <viua/program.h>
#include <viua/runtime/imports.h>
#include <viua/support/env.h>
#include <viua/support/string.h>
#include <viua/util/memory.h>

using viua::util::memory::aligned_read;
using viua::util::memory::aligned_write;

using viua::assembler::util::pretty_printer::ATTR_RESET;
using viua::assembler::util::pretty_printer::COLOR_FG_CYAN;
using viua::assembler::util::pretty_printer::COLOR_FG_LIGHT_GREEN;
using viua::assembler::util::pretty_printer::COLOR_FG_RED;
using viua::assembler::util::pretty_printer::COLOR_FG_WHITE;
using viua::assembler::util::pretty_printer::COLOR_FG_YELLOW;
using viua::assembler::util::pretty_printer::send_control_seq;


extern bool VERBOSE;


using Token = viua::cg::lex::Token;


template<class T> void bwrite(std::ofstream& out, T const& object)
{
    out.write(reinterpret_cast<const char*>(&object), sizeof(T));
}
static void strwrite(std::ofstream& out, std::string const& s)
{
    out.write(s.c_str(), static_cast<std::streamsize>(s.size()));
    out.put('\0');
}


/*  This is a mapping of instructions to their assembly functions.
 *  Used in the assembly() function.
 *
 *  It is suitable for all instructions which use three, simple register-index
 * operands.
 *
 *  BE WARNED!
 *  This mapping (and the assemble_three_intop_instruction() function) *greatly*
 * reduce the amount of code repetition in the assembler but is kinda black
 * voodoo magic...
 *
 *  NOTE TO FUTURE SELF:
 *  If you feel comfortable with taking pointers of member functions and calling
 * such things - go on. Otherwise, it may be better to leave this alone until
 * your have refreshed your memory. Here is isocpp.org's FAQ about pointers to
 * members (2015-01-17): https://isocpp.org/wiki/faq/pointers-to-members
 */
typedef Program& (Program::*ThreeIntopAssemblerFunction)(int_op,
                                                         int_op,
                                                         int_op);
const std::map<std::string, ThreeIntopAssemblerFunction>
    THREE_INTOP_ASM_FUNCTIONS = {
        {"and", &Program::opand},
        {"or", &Program::opor},

        {"capture", &Program::opcapture},
        {"capturecopy", &Program::opcapturecopy},
        {"capturemove", &Program::opcapturemove},
};


static auto compile(
    Program& program,
    std::vector<Token> const& tokens,
    std::map<std::string,
             std::remove_reference<decltype(tokens)>::type::size_type>& marks)
    -> Program&
{
    /** Compile instructions into bytecode using bytecode generation API.
     *
     */
    auto instruction = viua::internals::types::bytecode_size{0};
    for (auto i = decltype(tokens.size()){0}; i < tokens.size();) {
        i = viua::front::assembler::assemble_instruction(
            program, instruction, i, tokens, marks);
    }

    return program;
}


static auto strip_attributes(std::vector<viua::cg::lex::Token> const& tokens)
    -> std::vector<viua::cg::lex::Token>
{
    /*
     * After the codegen is ported to the new parser-driven way, the
     * strip-attributes code will not be needed.
     */
    std::remove_const_t<std::remove_reference_t<decltype(tokens)>> stripped;

    auto in_attributes = false;
    for (auto const& each : tokens) {
        if (each == "[[") {
            in_attributes = true;
            continue;
        } else if (each == "]]") {
            in_attributes = false;
            continue;
        }
        if (in_attributes) {
            continue;
        }
        stripped.push_back(each);
    }

    return stripped;
}

static void assemble(Program& program, std::vector<Token> const& tokens)
{
    /** Assemble instructions in lines into a program.
     *  This function first garthers required information about markers, named
     * registers and functions. Then, it passes all gathered data into
     * compilation function.
     *
     *  :params:
     *
     *  program         - Program object which will be used for assembling
     *  lines           - lines with instructions
     */
    auto marks = assembler::ce::getmarks(tokens);
    compile(program, tokens, marks);
}


static auto map_invocable_addresses(
    viua::internals::types::bytecode_size& starting_instruction,
    viua::front::assembler::Invocables const& blocks)
    -> std::map<std::string, viua::internals::types::bytecode_size>
{
    std::map<std::string, viua::internals::types::bytecode_size> addresses;
    for (auto const& name : blocks.names) {
        addresses[name] = starting_instruction;
        try {
            starting_instruction += viua::cg::tools::calculate_bytecode_size(
                blocks.tokens.at(name));
        } catch (std::out_of_range const& e) {
            throw("could not find block '" + name + "'");
        }
    }
    return addresses;
}

static auto write_code_blocks_section(
    std::ofstream& out,
    viua::front::assembler::Invocables const& blocks,
    std::vector<std::string> const& linked_block_names,
    viua::internals::types::bytecode_size block_bodies_size_so_far = 0)
    -> viua::internals::types::bytecode_size
{
    viua::internals::types::bytecode_size block_ids_section_size = 0;

    for (std::string name : blocks.names) {
        /*
         * Increase size of the block IDs section by
         * size of the block's name.
         */
        block_ids_section_size += name.size();

        /*
         * In bytecode, block names are stored as null-terminated ASCII strings.
         * std::string::size() does not include this terminating null byte, so
         * the size should be increased by 1.
         */
        // FIXME should be increased by size of byte as defined by Viua headers
        block_ids_section_size += 1;

        /*
         * Increase size of the block IDs section by size of address of
         * the block.
         */
        block_ids_section_size += sizeof(viua::internals::types::bytecode_size);
    }

    /*
     * Write out block IDs section's size.
     * Note that block IDs section may also include IDs and
     * addresses of statically linked blocks.
     */
    bwrite(out, block_ids_section_size);

    for (std::string name : blocks.names) {
        if (find(linked_block_names.begin(), linked_block_names.end(), name)
            != linked_block_names.end()) {
            continue;
        }

        /*
         * Write name of the block.
         * This name is used at runtime to find address of the block.
         */
        strwrite(out, name);

        /*
         * Mapped address must come after name.
         * This address is used at runtime to resolve offset from the beginning
         * of the loaded module at which the block's instructions begin.
         */
        // FIXME: use uncasted viua::internals::types::bytecode_size
        bwrite(out, block_bodies_size_so_far);

        /*
         * The 'block_bodies_size_so_far' variable must be incremented by
         * the actual size of block's bytecode size to give correct offset
         * for the next block.
         */
        try {
            block_bodies_size_so_far +=
                viua::cg::tools::calculate_bytecode_size(
                    blocks.tokens.at(name));
        } catch (std::out_of_range const& e) {
            throw("could not find block '" + name
                  + "' during address table write");
        }
    }

    return block_bodies_size_so_far;
}

static std::string get_main_function(
    std::vector<std::string> const& available_functions)
{
    auto main_function = std::string{};
    for (auto f : available_functions) {
        if (f == "main/0" or f == "main/1" or f == "main/2") {
            main_function = f;
            break;
        }
    }
    return main_function;
}

static void check_main_function(std::string const& main_function,
                                std::vector<Token> const& main_function_tokens)
{
    // Why three newlines?
    //
    // Here's why:
    //
    // - first newline is after the final 'return' instruction
    // - second newline is after the last-but-one instruction which should set
    // the return register
    // - third newline is the marker after which we look for the instruction
    // that will set the return register
    //
    // Example:
    //
    //   1st newline
    //         |
    //         |  2nd newline
    //         |   |
    //      nop    |
    //      izero 0
    //      return
    //            |
    //          3rd newline
    //
    // If these three newlines are found then the main function is considered
    // "full". Anything less, and things get suspicious. If there are two
    // newlines - maybe the function just returns something. If there is only
    // one newline - the main function is invalid, because there is no way to
    // correctly set the return register, and return from the function with one
    // instruction.
    //
    const int expected_newlines = 3;

    int found_newlines = 0;
    auto i             = main_function_tokens.size() - 1;
    while (i and found_newlines < expected_newlines) {
        if (main_function_tokens.at(i--) == "\n") {
            ++found_newlines;
        }
    }
    if (found_newlines >= expected_newlines) {
        // if found newlines number at least equals the expected number we
        // have to adjust token counter to skip past last required newline and
        // the token before it
        i += 2;
    }
    auto last_instruction = main_function_tokens.at(i);
    if (not(last_instruction == "copy" or last_instruction == "move"
            or last_instruction == "swap" or last_instruction == "izero"
            or last_instruction == "integer")) {
        throw viua::cg::lex::Invalid_syntax(
            last_instruction,
            ("main function does not return a value: " + main_function));
    }
    if (main_function_tokens.at(i + 1) != "%0") {
        throw viua::cg::lex::Invalid_syntax(
            last_instruction,
            ("main function does not return a value: " + main_function));
    }
    if (main_function_tokens.at(i + 2).original() == "\n") {
        throw viua::cg::lex::Invalid_syntax(
            last_instruction,
            "main function must explicitly return to local register set");
    }
    if (main_function_tokens.at(i + 2) != "local") {
        throw viua::cg::lex::Invalid_syntax(
            last_instruction,
            ("main function uses invalid register set to return a value: "
             + main_function_tokens.at(i + 2).str()))
            .add(main_function_tokens.at(i + 2));
    }
}

static auto generate_entry_function(
    viua::internals::types::bytecode_size bytes,
    std::map<std::string, viua::internals::types::bytecode_size>
        function_addresses,
    viua::front::assembler::Invocables& functions,
    std::string const& main_function,
    viua::internals::types::bytecode_size starting_instruction)
    -> viua::internals::types::bytecode_size
{
    auto entry_function_tokens = std::vector<Token>{};
    functions.names.emplace_back(ENTRY_FUNCTION_NAME);
    function_addresses[ENTRY_FUNCTION_NAME] = starting_instruction;

    // generate different instructions based on which main function variant
    // has been selected
    if (main_function == "main/0") {
        entry_function_tokens.emplace_back(0, 0, "frame");
        entry_function_tokens.emplace_back(0, 0, "%0");
        entry_function_tokens.emplace_back(0, 0, "arguments");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + sizeof(viua::internals::types::byte)
                 + sizeof(viua::internals::Register_sets)
                 + sizeof(viua::internals::types::register_index);
    } else if (main_function == "main/2") {
        entry_function_tokens.emplace_back(0, 0, "frame");
        entry_function_tokens.emplace_back(0, 0, "%2");
        entry_function_tokens.emplace_back(0, 0, "arguments");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + sizeof(viua::internals::types::byte)
                 + sizeof(viua::internals::Register_sets)
                 + sizeof(viua::internals::types::register_index);

        entry_function_tokens.emplace_back(0, 0, "izero");
        entry_function_tokens.emplace_back(0, 0, "%0");
        entry_function_tokens.emplace_back(0, 0, "local");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + sizeof(viua::internals::types::byte)
                 + sizeof(viua::internals::Register_sets)
                 + sizeof(viua::internals::types::register_index);

        // pop first element on the list of aruments
        entry_function_tokens.emplace_back(0, 0, "vpop");
        entry_function_tokens.emplace_back(0, 0, "%0");
        entry_function_tokens.emplace_back(0, 0, "local");
        entry_function_tokens.emplace_back(0, 0, "%1");
        entry_function_tokens.emplace_back(0, 0, "local");
        entry_function_tokens.emplace_back(0, 0, "%0");
        entry_function_tokens.emplace_back(0, 0, "local");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + 3 * sizeof(viua::internals::types::byte)
                 + 3 * sizeof(viua::internals::Register_sets)
                 + 3 * sizeof(viua::internals::types::register_index);

        // for parameter for main/2 is the name of the program
        entry_function_tokens.emplace_back(0, 0, "copy");
        entry_function_tokens.emplace_back(0, 0, "%0");
        entry_function_tokens.emplace_back(0, 0, "arguments");
        entry_function_tokens.emplace_back(0, 0, "%0");
        entry_function_tokens.emplace_back(0, 0, "local");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::Register_sets)
                 + 2 * sizeof(viua::internals::types::register_index);

        // second parameter for main/2 is the vector with the rest
        // of the commandl ine parameters
        entry_function_tokens.emplace_back(0, 0, "copy");
        entry_function_tokens.emplace_back(0, 0, "%1");
        entry_function_tokens.emplace_back(0, 0, "arguments");
        entry_function_tokens.emplace_back(0, 0, "%1");
        entry_function_tokens.emplace_back(0, 0, "local");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::Register_sets)
                 + 2 * sizeof(viua::internals::types::register_index);
    } else {
        // this is for default main function, i.e. `main/1` or
        // for custom main functions
        // FIXME: should custom main function be allowed?
        entry_function_tokens.emplace_back(0, 0, "frame");
        entry_function_tokens.emplace_back(0, 0, "%1");
        entry_function_tokens.emplace_back(0, 0, "arguments");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + sizeof(viua::internals::types::byte)
                 + sizeof(viua::internals::Register_sets)
                 + sizeof(viua::internals::types::register_index);

        entry_function_tokens.emplace_back(0, 0, "copy");
        entry_function_tokens.emplace_back(0, 0, "%0");
        entry_function_tokens.emplace_back(0, 0, "arguments");
        entry_function_tokens.emplace_back(0, 0, "%1");
        entry_function_tokens.emplace_back(0, 0, "local");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::Register_sets)
                 + 2 * sizeof(viua::internals::types::register_index);
    }

    entry_function_tokens.emplace_back(0, 0, "call");
    entry_function_tokens.emplace_back(0, 0, "%1");
    entry_function_tokens.emplace_back(0, 0, "local");
    entry_function_tokens.emplace_back(0, 0, main_function);
    entry_function_tokens.emplace_back(0, 0, "\n");
    bytes += sizeof(viua::internals::types::byte)
             + sizeof(viua::internals::types::byte)
             + sizeof(viua::internals::Register_sets)
             + sizeof(viua::internals::types::register_index);
    bytes += main_function.size() + 1;

    // then, register 1 is moved to register 0 so it counts as a return code
    entry_function_tokens.emplace_back(0, 0, "move");
    entry_function_tokens.emplace_back(0, 0, "%0");
    entry_function_tokens.emplace_back(0, 0, "local");
    entry_function_tokens.emplace_back(0, 0, "%1");
    entry_function_tokens.emplace_back(0, 0, "local");
    entry_function_tokens.emplace_back(0, 0, "\n");
    bytes += sizeof(viua::internals::types::byte)
             + 2 * sizeof(viua::internals::types::byte)
             + 2 * sizeof(viua::internals::Register_sets)
             + 2 * sizeof(viua::internals::types::register_index);

    entry_function_tokens.emplace_back(0, 0, "halt");
    entry_function_tokens.emplace_back(0, 0, "\n");
    bytes += sizeof(viua::internals::types::byte);

    functions.tokens[ENTRY_FUNCTION_NAME] = entry_function_tokens;

    return bytes;
}

namespace viua { namespace front { namespace assembler {
auto generate(std::vector<Token> const& tokens,
              viua::front::assembler::Invocables& functions,
              viua::front::assembler::Invocables& blocks,
              std::string const& filename,
              std::string& compilename,
              std::vector<std::string> const& static_imports,
              std::vector<std::pair<std::string, std::string>> const&
                  dynamic_imports_toplevel,
              viua::front::assembler::Compilation_flags const& flags) -> void
{
    //////////////////////////////
    // SETUP INITIAL BYTECODE SIZE
    auto bytes = viua::internals::types::bytecode_size{0};


    /////////////////////////
    // GET MAIN FUNCTION NAME
    auto main_function = get_main_function(functions.names);
    if (((flags.verbose and main_function != "main/1" and main_function != ""))
        and not flags.as_lib) {
        std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                  << send_control_seq(ATTR_RESET);
        std::cout << ": ";
        std::cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                  << send_control_seq(ATTR_RESET);
        std::cout << ": ";
        std::cout << "main function set to: ";
        std::cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << main_function
                  << send_control_seq(ATTR_RESET);
        std::cout << std::endl;
    }


    /////////////////////////////////////////
    // CHECK IF MAIN FUNCTION RETURNS A VALUE
    // FIXME: this is just a crude check - it does not acctually checks if these
    // instructions set 0 register this must be better implemented or we will
    // receive "function did not set return register" exceptions at runtime
    auto main_is_defined =
        (find(functions.names.begin(), functions.names.end(), main_function)
         != functions.names.end());
    if (not flags.as_lib and main_is_defined) {
        check_main_function(main_function, functions.tokens.at(main_function));
    }
    if (not main_is_defined and flags.verbose and not flags.as_lib) {
        std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                  << send_control_seq(ATTR_RESET);
        std::cout << ": ";
        std::cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                  << send_control_seq(ATTR_RESET);
        std::cout << ": ";
        std::cout << "main function (";
        std::cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << main_function
                  << send_control_seq(ATTR_RESET);
        std::cout
            << ") is not defined, deferring main function check to post-link "
               "phase"
            << std::endl;
    }


    /////////////////////////////////
    // MAP FUNCTIONS TO ADDRESSES AND
    // MAP BLOCKS TO ADDRESSES AND
    // SET STARTING INSTRUCTION
    auto starting_instruction = viua::internals::types::bytecode_size{
        0};  // the bytecode offset to first
             // executable instruction

    auto function_addresses =
        std::map<std::string, viua::internals::types::bytecode_size>{};
    auto block_addresses =
        std::map<std::string, viua::internals::types::bytecode_size>{};
    try {
        block_addresses = map_invocable_addresses(starting_instruction, blocks);
        function_addresses =
            map_invocable_addresses(starting_instruction, functions);
        bytes = viua::cg::tools::calculate_bytecode_size(tokens);
    } catch (std::string const& e) {
        throw("bytecode size calculation failed: " + e);
    }


    /////////////////////////////////////////////////////////
    // GATHER LINKS, GET THEIR SIZES AND ADJUST BYTECODE SIZE
    auto linked_libs_bytecode = std::vector<
        std::tuple<std::string,
                   viua::internals::types::bytecode_size,
                   std::unique_ptr<viua::internals::types::byte[]>>>{};
    auto linked_function_names        = std::vector<std::string>{};
    auto static_linked_function_names = std::vector<std::string>{};
    auto static_linked_block_names    = std::vector<std::string>{};
    auto visible_function_names       = std::vector<std::string>{};
    auto visible_block_names          = std::vector<std::string>{};
    auto linked_block_names           = std::vector<std::string>{};
    auto linked_libs_jumptables =
        std::map<std::string,
                 std::vector<viua::internals::types::bytecode_size>>{};

    // map of symbol names to name of the module the symbol came from
    auto symbol_sources = std::map<std::string, std::string>{};
    for (auto const& f : functions.names) {
        symbol_sources[f] = filename;
        visible_function_names.push_back(f);
        if (flags.verbose) {
            std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                      << send_control_seq(ATTR_RESET) << ": "
                      << send_control_seq(COLOR_FG_YELLOW) << "debug"
                      << send_control_seq(ATTR_RESET) << ": "
                      << "marking local-linked function \""
                      << send_control_seq(COLOR_FG_WHITE) << f
                      << send_control_seq(ATTR_RESET) << "\" (from \""
                      << send_control_seq(COLOR_FG_WHITE) << filename
                      << send_control_seq(ATTR_RESET) << "\") as visible"
                      << std::endl;
        }
    }

    auto links = std::vector<std::string>{};
    for (auto const& lnk : static_imports) {
        if (find(links.begin(), links.end(), lnk) == links.end()) {
            links.emplace_back(lnk);
        } else {
            throw("requested to link module '" + lnk + "' more than once");
        }
    }

    // gather all linked function names
    for (auto const& lnk : links) {
        auto loader = Loader{lnk};
        loader.load();

        {
            auto fn_names = loader.get_functions();
            if (fn_names.empty()) {
                std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                          << send_control_seq(ATTR_RESET) << ": "
                          << send_control_seq(COLOR_FG_RED) << "warning"
                          << send_control_seq(ATTR_RESET) << ": "
                          << "static-linked module \""
                          << send_control_seq(COLOR_FG_WHITE) << lnk
                          << send_control_seq(ATTR_RESET)
                          << "\" defines no functions\n";
            }

            for (auto const& fn : fn_names) {
                if (function_addresses.count(fn)) {
                    throw("duplicate symbol '" + fn + "' found when linking '"
                          + lnk + "' (previously found in '"
                          + symbol_sources.at(fn) + "')");
                }
            }
            for (auto const& fn : fn_names) {
                function_addresses[fn] = 0;  // for now we just build a list of
                                             // all available functions
                symbol_sources[fn] = lnk;
                linked_function_names.emplace_back(fn);
                static_linked_function_names.emplace_back(fn);
                visible_function_names.push_back(fn);

                if (flags.verbose) {
                    std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                              << send_control_seq(ATTR_RESET) << ": "
                              << send_control_seq(COLOR_FG_YELLOW) << "debug"
                              << send_control_seq(ATTR_RESET) << ": "
                              << "marking static-linked function \""
                              << send_control_seq(COLOR_FG_WHITE) << fn
                              << send_control_seq(ATTR_RESET) << "\" (from \""
                              << send_control_seq(COLOR_FG_WHITE) << lnk
                              << send_control_seq(ATTR_RESET)
                              << "\") as visible" << std::endl;
                }
            }
        }
        {
            auto bl_names = loader.get_blocks();
            for (auto const& bl : bl_names) {
                if (block_addresses.count(bl)) {
                    throw("duplicate symbol '" + bl + "' found when linking '"
                          + lnk + "' (previously found in '"
                          + symbol_sources.at(bl) + "')");
                }
            }
            for (auto const& bl : bl_names) {
                block_addresses[bl] = 0;  // for now we just build a list of all
                                          // available functions
                symbol_sources[bl] = lnk;
                linked_block_names.emplace_back(bl);
                static_linked_block_names.emplace_back(bl);
                visible_block_names.push_back(bl);

                if (flags.verbose) {
                    std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                              << send_control_seq(ATTR_RESET) << ": "
                              << send_control_seq(COLOR_FG_YELLOW) << "debug"
                              << send_control_seq(ATTR_RESET) << ": "
                              << "marking block \""
                              << send_control_seq(COLOR_FG_WHITE) << bl
                              << send_control_seq(ATTR_RESET) << "\" (from \""
                              << send_control_seq(COLOR_FG_WHITE) << lnk
                              << send_control_seq(ATTR_RESET)
                              << "\") as visible" << std::endl;
                }
            }
        }
    }

    auto dynamic_imports = std::vector<std::pair<std::string, std::string>>{};
    {
        auto already_imported = std::set<std::string>{};
        auto to_import = std::queue<std::pair<std::string, std::string>>{};

        /*
         * Populate the queue with modules that are dynamically imported by
         * modules that are direct imports of the current module.
         */
        {
            for (auto const& lnk : static_imports) {
                auto loader = Loader{lnk};
                loader.load();

                for (auto const& each : loader.dynamic_imports()) {
                    to_import.push({each, lnk});
                }
            }
            for (auto const& lnk : dynamic_imports_toplevel) {
                to_import.push({lnk.first, filename});
            }
        }

        while (not to_import.empty()) {
            auto const [module_name, dependency_of] =
                std::move(to_import.front());
            to_import.pop();

            if (already_imported.count(module_name)) {
                if (flags.verbose) {
                    std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                              << send_control_seq(ATTR_RESET) << ": "
                              << send_control_seq(COLOR_FG_YELLOW) << "debug"
                              << send_control_seq(ATTR_RESET) << ": "
                              << "dynamic imports of \""
                              << send_control_seq(COLOR_FG_WHITE) << module_name
                              << send_control_seq(ATTR_RESET)
                              << "\" (dependency of \""
                              << send_control_seq(COLOR_FG_WHITE)
                              << dependency_of << send_control_seq(ATTR_RESET)
                              << "\") were already gathered" << std::endl;
                }

                continue;
            }

            if (flags.verbose) {
                std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                          << send_control_seq(ATTR_RESET) << ": "
                          << send_control_seq(COLOR_FG_YELLOW) << "debug"
                          << send_control_seq(ATTR_RESET) << ": "
                          << "gathering dynamic imports of \""
                          << send_control_seq(COLOR_FG_WHITE) << module_name
                          << send_control_seq(ATTR_RESET)
                          << "\" (dependency of \""
                          << send_control_seq(COLOR_FG_WHITE) << dependency_of
                          << send_control_seq(ATTR_RESET) << "\")" << std::endl;
            }

            auto const module_path =
                viua::runtime::imports::find_module(module_name);
            if (not module_path.has_value()) {
                std::cerr << send_control_seq(COLOR_FG_WHITE) << filename
                          << send_control_seq(ATTR_RESET) << ':'
                          << send_control_seq(COLOR_FG_RED) << "error"
                          << send_control_seq(ATTR_RESET)
                          << ": did not find module \""
                          << send_control_seq(COLOR_FG_WHITE) << module_name
                          << send_control_seq(ATTR_RESET) << "\"\n";
                exit(1);
            }

            already_imported.insert(module_name);
            dynamic_imports.push_back({module_name, module_path->second});

            using viua::runtime::imports::Module_type;
            if (module_path->first == Module_type::Native) {
                if (flags.verbose) {
                    std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                              << send_control_seq(ATTR_RESET) << ": ";
                    std::cout << "import of FFI module: \""
                              << send_control_seq(COLOR_FG_WHITE) << module_name
                              << send_control_seq(ATTR_RESET)
                              << "\" (found in \"" << module_path->second
                              << "\"\n";
                }
                continue;
            }

            auto loaded_module = Loader{module_path->second};
            loaded_module.load();

            for (auto const& each : loaded_module.dynamic_imports()) {
                if (not already_imported.count(each)) {
                    to_import.push({each, module_name});
                }
            }
        }

        for (auto const& lnk : dynamic_imports) {
            if (lnk.second.substr(lnk.second.size() - 3) == ".so") {
                if (flags.verbose) {
                    std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                              << send_control_seq(ATTR_RESET) << ": ";
                    std::cout << "import of FFI module: \""
                              << send_control_seq(COLOR_FG_WHITE) << lnk.first
                              << send_control_seq(ATTR_RESET)
                              << "\" (found in \"" << lnk.second << "\"\n";
                }
                continue;
            }

            auto loader = Loader{lnk.second};
            loader.load();

            {
                auto fn_names = loader.get_functions();
                for (auto fn : fn_names) {
                    /*
                     * For functions found in current module, and in statically
                     * imported modules.
                     */
                    if (function_addresses.count(fn)) {
                        throw("duplicate symbol '" + fn
                              + "' found when linking '" + lnk.first
                              + "' (previously found in '"
                              + symbol_sources.at(fn) + "')");
                    }

                    /*
                     * For functions found dynamically imported modules.
                     */
                    if (std::find(linked_function_names.begin(),
                                  linked_function_names.end(),
                                  fn)
                        != linked_function_names.end()) {
                        throw("duplicate symbol '" + fn
                              + "' found when linking '" + lnk.first
                              + "' (previously found in '"
                              + symbol_sources.at(fn) + "')");
                    }
                }
                for (auto const& fn : fn_names) {
                    symbol_sources[fn] = lnk.first;
                    linked_function_names.emplace_back(fn);
                    visible_function_names.push_back(fn);

                    if (flags.verbose) {
                        std::cout
                            << send_control_seq(COLOR_FG_WHITE) << filename
                            << send_control_seq(ATTR_RESET) << ": "
                            << send_control_seq(COLOR_FG_YELLOW) << "debug"
                            << send_control_seq(ATTR_RESET) << ": "
                            << "marking dynamic-linked function \""
                            << send_control_seq(COLOR_FG_WHITE) << fn
                            << send_control_seq(ATTR_RESET) << "\" (from \""
                            << send_control_seq(COLOR_FG_WHITE) << lnk.first
                            << send_control_seq(ATTR_RESET) << "\") as visible"
                            << std::endl;
                    }
                }
            }
            {
                auto bl_names = loader.get_blocks();
                for (auto bl : bl_names) {
                    /*
                     * For blocks found in current module, and in statically
                     * imported modules.
                     */
                    if (block_addresses.count(bl)) {
                        throw("duplicate symbol '" + bl
                              + "' found when linking '" + lnk.first
                              + "' (previously found in '"
                              + symbol_sources.at(bl) + "')");
                    }

                    /*
                     * For blocks found dynamically imported modules.
                     */
                    if (std::find(linked_block_names.begin(),
                                  linked_block_names.end(),
                                  bl)
                        != linked_block_names.end()) {
                        throw("duplicate symbol '" + bl
                              + "' found when linking '" + lnk.first
                              + "' (previously found in '"
                              + symbol_sources.at(bl) + "')");
                    }
                }
                for (auto const& bl : bl_names) {
                    symbol_sources[bl] = lnk.first;
                    linked_block_names.emplace_back(bl);
                    visible_block_names.push_back(bl);

                    if (flags.verbose) {
                        std::cout
                            << send_control_seq(COLOR_FG_WHITE) << filename
                            << send_control_seq(ATTR_RESET) << ": "
                            << send_control_seq(COLOR_FG_YELLOW) << "debug"
                            << send_control_seq(ATTR_RESET) << ": "
                            << "marking block \""
                            << send_control_seq(COLOR_FG_WHITE) << bl
                            << send_control_seq(ATTR_RESET) << "\" (from \""
                            << send_control_seq(COLOR_FG_WHITE) << lnk.first
                            << send_control_seq(ATTR_RESET) << "\") as visible"
                            << std::endl;
                    }
                }
            }
        }
    }


    //////////////////////////////////////////////////////////////
    // EXTEND FUNCTION NAMES VECTOR WITH NAMES OF LINKED FUNCTIONS
    auto local_function_names = functions.names;
    for (auto const& name : static_linked_function_names) {
        functions.names.emplace_back(name);
    }
    auto local_block_names = blocks.names;
    for (auto const& name : static_linked_block_names) {
        blocks.names.emplace_back(name);
    }


    if (not flags.as_lib) {
        /*
         * Check if our initial guess for main function is correct and
         * detect some main-function-related errors
         */
        auto main_function_found = std::vector<std::string>{};
        for (auto f : functions.names) {
            if (f == "main/0" or f == "main/1" or f == "main/2") {
                main_function_found.emplace_back(f);
            }
        }
        if (main_function_found.size() > 1) {
            for (auto f : main_function_found) {
                std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                          << send_control_seq(ATTR_RESET);
                std::cout << ": ";
                std::cout << send_control_seq(COLOR_FG_CYAN) << "note"
                          << send_control_seq(ATTR_RESET);
                std::cout << ": ";
                std::cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << f
                          << send_control_seq(ATTR_RESET);
                std::cout << " function found in module ";
                std::cout << send_control_seq(COLOR_FG_WHITE)
                          << symbol_sources.at(f)
                          << send_control_seq(ATTR_RESET);
                std::cout << std::endl;
            }
            throw "more than one candidate for main function";
        } else if (main_function_found.size() == 0) {
            throw "main function is not defined";
        }
        main_function = main_function_found[0];
    }


    //////////////////////////
    // GENERATE ENTRY FUNCTION
    if (not flags.as_lib) {
        bytes = generate_entry_function(bytes,
                                        function_addresses,
                                        functions,
                                        main_function,
                                        starting_instruction);
    }


    auto current_link_offset = bytes;
    auto linked_module_offsets =
        std::map<std::string, viua::internals::types::bytecode_size>{};
    for (auto lnk : links) {
        auto loader = Loader{lnk};
        loader.load();

        auto lib_jumps              = loader.get_jumps();
        linked_libs_jumptables[lnk] = lib_jumps;

        {
            auto fn_names     = loader.get_functions();
            auto fn_addresses = loader.get_function_addresses();
            for (auto const& fn : fn_names) {
                function_addresses[fn] =
                    fn_addresses.at(fn) + current_link_offset;
            }
        }
        {
            auto bl_names     = loader.get_blocks();
            auto bl_addresses = loader.get_block_addresses();
            for (auto const& bl : bl_names) {
                block_addresses[bl] = bl_addresses.at(bl) + current_link_offset;
            }
        }

        linked_libs_bytecode.emplace_back(
            lnk, loader.get_bytecode_size(), loader.get_bytecode());
        linked_module_offsets.insert({lnk, current_link_offset});
        bytes += loader.get_bytecode_size();
        current_link_offset += loader.get_bytecode_size();
    }

    {
        std::map<viua::internals::types::bytecode_size, std::string>
            address_of_fn;
        for (auto const& [name, addr] : function_addresses) {
            if (not address_of_fn.insert({addr, name}).second) {
                std::cerr << send_control_seq(COLOR_FG_WHITE) << filename
                          << send_control_seq(ATTR_RESET);
                std::cerr << ": ";
                std::cerr << send_control_seq(COLOR_FG_RED) << "error"
                          << send_control_seq(ATTR_RESET);
                std::cerr << ": function \"";
                std::cerr << send_control_seq(COLOR_FG_WHITE) << name
                          << send_control_seq(ATTR_RESET)
                          << "\" loaded at the same address as \"";
                std::cerr << send_control_seq(COLOR_FG_WHITE)
                          << address_of_fn.at(addr) << '"'
                          << send_control_seq(ATTR_RESET) << '\n';
                exit(1);
            }
        }
    }
    {
        std::map<viua::internals::types::bytecode_size, std::string>
            address_of_bl;
        for (auto const& [name, addr] : block_addresses) {
            if (not address_of_bl.insert({addr, name}).second) {
                std::cerr << send_control_seq(COLOR_FG_WHITE) << filename
                          << send_control_seq(ATTR_RESET);
                std::cerr << ": ";
                std::cerr << send_control_seq(COLOR_FG_RED) << "error"
                          << send_control_seq(ATTR_RESET);
                std::cerr << ": block \"";
                std::cerr << send_control_seq(COLOR_FG_WHITE) << name
                          << send_control_seq(ATTR_RESET)
                          << "\" loaded at the same address as \"";
                std::cerr << send_control_seq(COLOR_FG_WHITE)
                          << address_of_bl.at(addr) << '"'
                          << send_control_seq(ATTR_RESET) << '\n';
                exit(1);
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////
    // AFTER HAVING OBTAINED LINKED NAMES, IT IS POSSIBLE TO VERIFY CALLS AND
    // CALLABLE (FUNCTIONS, CLOSURES, ETC.) CREATIONS
    ::assembler::verify::function_calls_are_defined(
        tokens, visible_function_names, functions.signatures);
    ::assembler::verify::callable_creations(
        tokens, visible_function_names, functions.signatures);


    /////////////////////////////
    // REPORT TOTAL BYTECODE SIZE
    if (flags.verbose and linked_function_names.size() != 0) {
        std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                  << send_control_seq(ATTR_RESET);
        std::cout << ": ";
        std::cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                  << send_control_seq(ATTR_RESET);
        std::cout << ": ";
        std::cout << "total required bytes: " << bytes << " bytes" << std::endl;
    }


    ///////////////////////////
    // REPORT FIRST INSTRUCTION
    if (flags.verbose and not flags.as_lib) {
        std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                  << send_control_seq(ATTR_RESET);
        std::cout << ": ";
        std::cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                  << send_control_seq(ATTR_RESET);
        std::cout << ": ";
        std::cout << "first instruction pointer: " << starting_instruction
                  << std::endl;
    }


    ////////////////////
    // CREATE JUMP TABLE
    auto jump_table = std::vector<viua::internals::types::bytecode_size>{};


    /////////////////////////////////////////////////////////
    // GENERATE BYTECODE OF LOCAL FUNCTIONS AND BLOCKS
    //
    // BYTECODE IS GENERATED HERE BUT NOT YET WRITTEN TO FILE
    // THIS MUST BE GENERATED HERE TO OBTAIN FILL JUMP TABLE
    auto functions_bytecode =
        std::map<std::string,
                 std::tuple<viua::internals::types::bytecode_size,
                            std::unique_ptr<viua::internals::types::byte[]>>>{};
    auto block_bodies_bytecode =
        std::map<std::string,
                 std::tuple<viua::internals::types::bytecode_size,
                            std::unique_ptr<viua::internals::types::byte[]>>>{};
    auto functions_section_size    = viua::internals::types::bytecode_size{0};
    auto block_bodies_section_size = viua::internals::types::bytecode_size{0};

    auto jump_positions =
        std::vector<std::tuple<viua::internals::types::bytecode_size,
                               viua::internals::types::bytecode_size>>{};

    for (auto const& name : blocks.names) {
        // do not generate bytecode for blocks that were linked
        if (std::find(
                linked_block_names.begin(), linked_block_names.end(), name)
            != linked_block_names.end()) {
            continue;
        }

        if (flags.verbose) {
            std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                      << send_control_seq(ATTR_RESET);
            std::cout << ": ";
            std::cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                      << send_control_seq(ATTR_RESET);
            std::cout << ": ";
            std::cout << "generating bytecode for block \"";
            std::cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name
                      << send_control_seq(ATTR_RESET);
            std::cout << '"';
        }
        auto fun_bytes = viua::internals::types::bytecode_size{0};
        try {
            fun_bytes = viua::cg::tools::calculate_bytecode_size(
                blocks.tokens.at(name));
            if (flags.verbose) {
                std::cout << " (" << fun_bytes << " bytes at byte "
                          << block_bodies_section_size << ')' << std::endl;
            }
        } catch (std::string const& e) {
            throw("failed block size count (during pre-assembling): " + e);
        } catch (std::out_of_range const& e) {
            throw("in block '" + name + "': " + e.what());
        }

        auto blok = Program{fun_bytes};
        try {
            assemble(blok, strip_attributes(blocks.tokens.at(name)));
        } catch (std::string const& e) {
            throw("in block '" + name + "': " + e);
        } catch (const char*& e) {
            throw("in block '" + name + "': " + e);
        } catch (std::out_of_range const& e) {
            throw("in block '" + name + "': " + e.what());
        }

        auto jumps = blok.jumps();

        auto local_jumps =
            std::vector<std::tuple<viua::internals::types::bytecode_size,
                                   viua::internals::types::bytecode_size>>{};
        for (auto jmp : jumps) {
            local_jumps.emplace_back(jmp, block_bodies_section_size);
        }
        blok.calculate_jumps(local_jumps, blocks.tokens.at(name));

        auto btcode = blok.bytecode();

        // store generated bytecode fragment for future use (we must not yet
        // write it to the file to conform to bytecode format)
        block_bodies_bytecode[name] =
            std::tuple<viua::internals::types::bytecode_size, decltype(btcode)>(
                blok.size(), std::move(btcode));

        // extend jump table with jumps from current block
        for (auto i = decltype(jumps)::size_type{0}; i < jumps.size(); ++i) {
            auto const jmp = jumps[i];
            jump_table.emplace_back(jmp + block_bodies_section_size);
        }

        block_bodies_section_size += blok.size();
    }

    // functions section size, must be offset by the size of block section
    functions_section_size = block_bodies_section_size;

    for (auto const& name : functions.names) {
        // do not generate bytecode for functions that were linked
        if (std::find(linked_function_names.begin(),
                      linked_function_names.end(),
                      name)
            != linked_function_names.end()) {
            continue;
        }

        auto fun_bytes = viua::internals::types::bytecode_size{0};
        try {
            fun_bytes = viua::cg::tools::calculate_bytecode_size(
                functions.tokens.at(name));
        } catch (std::string const& e) {
            throw("failed function size count (during pre-assembling): " + e);
        } catch (std::out_of_range const& e) {
            throw e.what();
        }

        auto func = Program{fun_bytes};
        try {
            assemble(func, strip_attributes(functions.tokens.at(name)));
        } catch (std::string const& e) {
            auto const msg =
                ("in function '" + send_control_seq(COLOR_FG_LIGHT_GREEN) + name
                 + send_control_seq(ATTR_RESET) + "': " + e);
            throw msg;
        } catch (const char*& e) {
            auto const msg =
                ("in function '" + send_control_seq(COLOR_FG_LIGHT_GREEN) + name
                 + send_control_seq(ATTR_RESET) + "': " + e);
            throw msg;
        } catch (std::out_of_range const& e) {
            auto const msg =
                ("in function '" + send_control_seq(COLOR_FG_LIGHT_GREEN) + name
                 + send_control_seq(ATTR_RESET) + "': " + e.what());
            throw msg;
        }

        auto jumps = func.jumps();

        auto local_jumps =
            std::vector<std::tuple<viua::internals::types::bytecode_size,
                                   viua::internals::types::bytecode_size>>{};
        for (auto i = decltype(jumps)::size_type{0}; i < jumps.size(); ++i) {
            viua::internals::types::bytecode_size jmp = jumps[i];
            local_jumps.emplace_back(jmp, functions_section_size);
        }
        func.calculate_jumps(local_jumps, functions.tokens.at(name));

        auto btcode = func.bytecode();

        // store generated bytecode fragment for future use (we must not yet
        // write it to the file to conform to bytecode format)
        functions_bytecode[name] =
            std::tuple<viua::internals::types::bytecode_size, decltype(btcode)>{
                func.size(), std::move(btcode)};

        // extend jump table with jumps from current function
        for (decltype(jumps)::size_type i = 0; i < jumps.size(); ++i) {
            viua::internals::types::bytecode_size jmp = jumps[i];
            jump_table.emplace_back(jmp + functions_section_size);
        }

        functions_section_size += func.size();
    }


    ////////////////////////////////////////
    // CREATE OFSTREAM TO WRITE BYTECODE OUT
    auto out = std::ofstream{compilename, std::ios::out | std::ios::binary};

    out.write(VIUA_MAGIC_NUMBER, sizeof(char) * 5);
    if (flags.as_lib) {
        out.put(static_cast<Viua_binary_type>(VIUA_LINKABLE));
    } else {
        out.put(static_cast<Viua_binary_type>(VIUA_EXECUTABLE));
    }


    /////////////////////////////////////////////////////////////
    // WRITE META-INFORMATION MAP
    auto meta_information_map =
        viua::assembler::frontend::gather_meta_information(tokens);
    viua::internals::types::bytecode_size meta_information_map_size = 0;
    for (auto each : meta_information_map) {
        meta_information_map_size +=
            (each.first.size() + each.second.size() + 2);
    }

    bwrite(out, meta_information_map_size);
    for (auto each : meta_information_map) {
        strwrite(out, each.first);
        strwrite(out, each.second);
    }


    //////////////////////////
    // IF ASSEMBLING A LIBRARY
    // WRITE OUT JUMP TABLE
    if (flags.as_lib) {
        viua::internals::types::bytecode_size total_jumps = jump_table.size();
        bwrite(out, total_jumps);

        viua::internals::types::bytecode_size jmp;
        for (decltype(total_jumps) i = 0; i < total_jumps; ++i) {
            jmp = jump_table[i];
            bwrite(out, jmp);
        }
    }


    /////////////////////////////////////////////////////////////
    // WRITE EXTERNAL FUNCTION SIGNATURES
    {
        viua::internals::types::bytecode_size signatures_section_size = 0;
        for (auto const& each : functions.signatures) {
            signatures_section_size += (each.size() + 1);  // +1 for null byte
                                                           // after each
                                                           // signature
        }
        bwrite(out, signatures_section_size);
        for (auto const& each : functions.signatures) {
            strwrite(out, each);
        }
    }


    /////////////////////////////////////////////////////////////
    // WRITE EXTERNAL BLOCK SIGNATURES
    {
        viua::internals::types::bytecode_size signatures_section_size = 0;
        for (auto const& each : blocks.signatures) {
            signatures_section_size += (each.size() + 1);  // +1 for null byte
                                                           // after each
                                                           // signature
        }
        bwrite(out, signatures_section_size);
        for (auto const& each : blocks.signatures) {
            strwrite(out, each);
        }
    }


    /////////////////////////////////////////////////////////////
    // WRITE DYNAMIC IMPORTS SECTION
    {
        auto dynamic_imports_section_size =
            viua::internals::types::bytecode_size{0};
        for (auto const& each : dynamic_imports) {
            auto const& module_name = each.first;
            dynamic_imports_section_size +=
                (module_name.size() + 1);  // +1 for null byte after
        }
        bwrite(out, dynamic_imports_section_size);
        for (auto const& each : dynamic_imports) {
            auto const& module_name = each.first;
            strwrite(out, module_name);

            if (flags.verbose) {
                std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                          << send_control_seq(ATTR_RESET);
                std::cout << ": ";
                std::cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                          << send_control_seq(ATTR_RESET);
                std::cout << ": ";
                std::cout << "dynamically linked module \"";
                std::cout << send_control_seq(COLOR_FG_WHITE) << module_name
                          << send_control_seq(ATTR_RESET);
                std::cout << "\" scheduled for loading at startup" << std::endl;
            }
        }
    }


    /////////////////////////////////////////////////////////////
    // WRITE BLOCK AND FUNCTION ENTRY POINT ADDRESSES TO BYTECODE
    viua::internals::types::bytecode_size functions_size_so_far =
        write_code_blocks_section(out, blocks, linked_block_names);
    for (auto const& name : static_linked_block_names) {
        strwrite(out, name);
        // mapped address must come after name
        viua::internals::types::bytecode_size address = block_addresses[name];
        bwrite(out, address);
    }

    write_code_blocks_section(
        out, functions, static_linked_function_names, functions_size_so_far);
    for (auto const& name : static_linked_function_names) {
        strwrite(out, name);
        // mapped address must come after name
        viua::internals::types::bytecode_size address =
            function_addresses[name];
        bwrite(out, address);
    }


    //////////////////////
    // WRITE BYTECODE SIZE
    bwrite(out, bytes);

    auto program_bytecode =
        std::make_unique<viua::internals::types::byte[]>(bytes);
    viua::internals::types::bytecode_size program_bytecode_used = 0;

    ////////////////////////////////////////////////////
    // WRITE BYTECODE OF LOCAL BLOCKS TO BYTECODE BUFFER
    for (auto const& name : blocks.names) {
        // linked blocks are to be inserted later
        if (find(linked_block_names.begin(), linked_block_names.end(), name)
            != linked_block_names.end()) {
            continue;
        }

        auto const fun_size = std::get<0>(block_bodies_bytecode[name]);
        auto const fun_bytecode =
            std::get<1>(block_bodies_bytecode[name]).get();

        for (viua::internals::types::bytecode_size i = 0; i < fun_size; ++i) {
            program_bytecode[program_bytecode_used + i] = fun_bytecode[i];
        }
        program_bytecode_used += fun_size;
    }


    ///////////////////////////////////////////////////////
    // WRITE BYTECODE OF LOCAL FUNCTIONS TO BYTECODE BUFFER
    for (auto const& name : functions.names) {
        // linked functions are to be inserted later
        if (find(static_linked_function_names.begin(),
                 static_linked_function_names.end(),
                 name)
            != static_linked_function_names.end()) {
            continue;
        }

        auto const fun_size     = std::get<0>(functions_bytecode[name]);
        auto const fun_bytecode = std::get<1>(functions_bytecode[name]).get();

        for (viua::internals::types::bytecode_size i = 0; i < fun_size; ++i) {
            program_bytecode[program_bytecode_used + i] = fun_bytecode[i];
        }
        program_bytecode_used += fun_size;
    }

    ////////////////////////////////////
    // WRITE STATICALLY LINKED LIBRARIES
    for (auto& lnk : linked_libs_bytecode) {
        auto const lib_name        = std::get<0>(lnk);
        auto const linked_bytecode = std::get<2>(lnk).get();
        auto const linked_size     = std::get<1>(lnk);

        auto const bytes_offset = linked_module_offsets.at(lib_name);

        if (flags.verbose) {
            std::cout << send_control_seq(COLOR_FG_WHITE) << filename
                      << send_control_seq(ATTR_RESET);
            std::cout << ": ";
            std::cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                      << send_control_seq(ATTR_RESET);
            std::cout << ": ";
            std::cout << "linked module \"";
            std::cout << send_control_seq(COLOR_FG_WHITE) << lib_name
                      << send_control_seq(ATTR_RESET);
            std::cout << "\" written at offset " << bytes_offset << std::endl;
        }

        auto linked_jumptable =
            std::vector<viua::internals::types::bytecode_size>{};
        try {
            linked_jumptable = linked_libs_jumptables[lib_name];
        } catch (std::out_of_range const& e) {
            throw("[linker] could not find jumptable for '" + lib_name
                  + "' (maybe not loaded?)");
        }

        viua::internals::types::bytecode_size jmp, jmp_target;
        for (auto i = decltype(linked_jumptable)::size_type{0};
             i < linked_jumptable.size();
             ++i) {
            jmp                      = linked_jumptable[i];
            aligned_read(jmp_target) = (linked_bytecode + jmp);
            aligned_write(linked_bytecode + jmp) += bytes_offset;
        }

        for (auto i = decltype(linked_size){0}; i < linked_size; ++i) {
            program_bytecode[program_bytecode_used + i] = linked_bytecode[i];
        }
        program_bytecode_used += linked_size;
    }

    out.write(reinterpret_cast<const char*>(program_bytecode.get()),
              static_cast<std::streamsize>(bytes));
    out.close();
}
}}}  // namespace viua::front::assembler
