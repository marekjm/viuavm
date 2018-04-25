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
#include <sstream>
#include <viua/assembler/util/pretty_printer.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/cg/tokenizer.h>
#include <viua/cg/tools.h>
#include <viua/front/asm.h>
#include <viua/loader.h>
#include <viua/machine.h>
#include <viua/program.h>
#include <viua/support/env.h>
#include <viua/support/string.h>
#include <viua/util/memory.h>
using namespace std;

using viua::util::memory::aligned_read;
using viua::util::memory::aligned_write;

using viua::assembler::util::pretty_printer::ATTR_RESET;
using viua::assembler::util::pretty_printer::COLOR_FG_CYAN;
using viua::assembler::util::pretty_printer::COLOR_FG_LIGHT_GREEN;
using viua::assembler::util::pretty_printer::COLOR_FG_WHITE;
using viua::assembler::util::pretty_printer::COLOR_FG_YELLOW;
using viua::assembler::util::pretty_printer::send_control_seq;


extern bool VERBOSE;
extern bool DEBUG;
extern bool SCREAM;


using Token = viua::cg::lex::Token;


template<class T> void bwrite(ofstream& out, const T& object) {
    out.write(reinterpret_cast<const char*>(&object), sizeof(T));
}
static void strwrite(ofstream& out, const string& s) {
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
const map<string, ThreeIntopAssemblerFunction> THREE_INTOP_ASM_FUNCTIONS = {
    {"and", &Program::opand},
    {"or", &Program::opor},

    {"capture", &Program::opcapture},
    {"capturecopy", &Program::opcapturecopy},
    {"capturemove", &Program::opcapturemove},
};


static Program& compile(
    Program& program,
    const vector<Token>& tokens,
    map<string, std::remove_reference<decltype(tokens)>::type::size_type>&
        marks) {
    /** Compile instructions into bytecode using bytecode generation API.
     *
     */
    viua::internals::types::bytecode_size instruction = 0;
    for (decltype(tokens.size()) i = 0; i < tokens.size();) {
        i = assemble_instruction(program, instruction, i, tokens, marks);
    }

    return program;
}


static auto strip_attributes(vector<viua::cg::lex::Token> const& tokens)
    -> vector<viua::cg::lex::Token> {
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

static void assemble(Program& program, const vector<Token>& tokens) {
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


static map<string, viua::internals::types::bytecode_size>
map_invocable_addresses(
    viua::internals::types::bytecode_size& starting_instruction,
    const invocables_t& blocks) {
    map<string, viua::internals::types::bytecode_size> addresses;
    for (string name : blocks.names) {
        addresses[name] = starting_instruction;
        try {
            starting_instruction += viua::cg::tools::calculate_bytecode_size2(
                blocks.tokens.at(name));
        } catch (const std::out_of_range& e) {
            throw("could not find block '" + name + "'");
        }
    }
    return addresses;
}

static viua::internals::types::bytecode_size write_code_blocks_section(
    ofstream& out,
    const invocables_t& blocks,
    const vector<string>& linked_block_names,
    viua::internals::types::bytecode_size block_bodies_size_so_far = 0) {
    viua::internals::types::bytecode_size block_ids_section_size = 0;

    for (string name : blocks.names) {
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

    for (string name : blocks.names) {
        if (DEBUG) {
            cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << "message"
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "writing block '";
            cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name
                 << send_control_seq(ATTR_RESET);
            cout << "' to block address table";
        }
        if (find(linked_block_names.begin(), linked_block_names.end(), name)
            != linked_block_names.end()) {
            if (DEBUG) {
                cout << ": delayed" << endl;
            }
            continue;
        }
        if (DEBUG) {
            cout << endl;
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
                viua::cg::tools::calculate_bytecode_size2(
                    blocks.tokens.at(name));
        } catch (const std::out_of_range& e) {
            throw("could not find block '" + name
                  + "' during address table write");
        }
    }

    return block_bodies_size_so_far;
}

static string get_main_function(const vector<string>& available_functions) {
    string main_function = "";
    for (auto f : available_functions) {
        if (f == "main/0" or f == "main/1" or f == "main/2") {
            main_function = f;
            break;
        }
    }
    return main_function;
}

static void check_main_function(const string& main_function,
                                const vector<Token>& main_function_tokens) {
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
        throw viua::cg::lex::InvalidSyntax(
            last_instruction,
            ("main function does not return a value: " + main_function));
    }
    if (main_function_tokens.at(i + 1) != "%0") {
        throw viua::cg::lex::InvalidSyntax(
            last_instruction,
            ("main function does not return a value: " + main_function));
    }
    if (main_function_tokens.at(i + 2).original() == "\n") {
        throw viua::cg::lex::InvalidSyntax(
            last_instruction,
            "main function must explicitly return to local register set");
    }
    if (main_function_tokens.at(i + 2) != "local") {
        throw viua::cg::lex::InvalidSyntax(
            last_instruction,
            ("main function uses invalid register set to return a value: "
             + main_function_tokens.at(i + 2).str()))
            .add(main_function_tokens.at(i + 2));
    }
}

static viua::internals::types::bytecode_size generate_entry_function(
    viua::internals::types::bytecode_size bytes,
    map<string, viua::internals::types::bytecode_size> function_addresses,
    invocables_t& functions,
    const string& main_function,
    viua::internals::types::bytecode_size starting_instruction) {
    if (DEBUG) {
        cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << "message"
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "generating ";
        cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << ENTRY_FUNCTION_NAME
             << send_control_seq(ATTR_RESET);
        cout << " function" << endl;
    }

    vector<Token> entry_function_tokens;
    functions.names.emplace_back(ENTRY_FUNCTION_NAME);
    function_addresses[ENTRY_FUNCTION_NAME] = starting_instruction;

    // generate different instructions based on which main function variant
    // has been selected
    if (main_function == "main/0") {
        entry_function_tokens.emplace_back(0, 0, "frame");
        entry_function_tokens.emplace_back(0, 0, "%0");
        entry_function_tokens.emplace_back(0, 0, "%16");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::RegisterSets)
                 + 2 * sizeof(viua::internals::types::register_index);
    } else if (main_function == "main/2") {
        entry_function_tokens.emplace_back(0, 0, "frame");
        entry_function_tokens.emplace_back(0, 0, "%2");
        entry_function_tokens.emplace_back(0, 0, "%16");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::RegisterSets)
                 + 2 * sizeof(viua::internals::types::register_index);

        entry_function_tokens.emplace_back(0, 0, "izero");
        entry_function_tokens.emplace_back(0, 0, "%0");
        entry_function_tokens.emplace_back(0, 0, "local");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + sizeof(viua::internals::types::byte)
                 + sizeof(viua::internals::RegisterSets)
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
                 + 3 * sizeof(viua::internals::RegisterSets)
                 + 3 * sizeof(viua::internals::types::register_index);

        // for parameter for main/2 is the name of the program
        entry_function_tokens.emplace_back(0, 0, "param");
        entry_function_tokens.emplace_back(0, 0, "%0");
        entry_function_tokens.emplace_back(0, 0, "%0");
        entry_function_tokens.emplace_back(0, 0, "local");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::RegisterSets)
                 + 2 * sizeof(viua::internals::types::register_index);

        // second parameter for main/2 is the vector with the rest
        // of the commandl ine parameters
        entry_function_tokens.emplace_back(0, 0, "param");
        entry_function_tokens.emplace_back(0, 0, "%1");
        entry_function_tokens.emplace_back(0, 0, "%1");
        entry_function_tokens.emplace_back(0, 0, "local");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::RegisterSets)
                 + 2 * sizeof(viua::internals::types::register_index);
    } else {
        // this is for default main function, i.e. `main/1` or
        // for custom main functions
        // FIXME: should custom main function be allowed?
        entry_function_tokens.emplace_back(0, 0, "frame");
        entry_function_tokens.emplace_back(0, 0, "%1");
        entry_function_tokens.emplace_back(0, 0, "%16");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::RegisterSets)
                 + 2 * sizeof(viua::internals::types::register_index);

        entry_function_tokens.emplace_back(0, 0, "param");
        entry_function_tokens.emplace_back(0, 0, "%0");
        entry_function_tokens.emplace_back(0, 0, "%1");
        entry_function_tokens.emplace_back(0, 0, "local");
        entry_function_tokens.emplace_back(0, 0, "\n");
        bytes += sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::types::byte)
                 + 2 * sizeof(viua::internals::RegisterSets)
                 + 2 * sizeof(viua::internals::types::register_index);
    }

    entry_function_tokens.emplace_back(0, 0, "call");
    entry_function_tokens.emplace_back(0, 0, "%1");
    entry_function_tokens.emplace_back(0, 0, "local");
    entry_function_tokens.emplace_back(0, 0, main_function);
    entry_function_tokens.emplace_back(0, 0, "\n");
    bytes += sizeof(viua::internals::types::byte)
             + sizeof(viua::internals::types::byte)
             + sizeof(viua::internals::RegisterSets)
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
             + 2 * sizeof(viua::internals::RegisterSets)
             + 2 * sizeof(viua::internals::types::register_index);

    entry_function_tokens.emplace_back(0, 0, "halt");
    entry_function_tokens.emplace_back(0, 0, "\n");
    bytes += sizeof(viua::internals::types::byte);

    functions.tokens[ENTRY_FUNCTION_NAME] = entry_function_tokens;

    return bytes;
}

void generate(vector<Token> const& tokens,
              invocables_t& functions,
              invocables_t& blocks,
              const string& filename,
              string& compilename,
              const vector<string>& commandline_given_links,
              const compilationflags_t& flags) {
    //////////////////////////////
    // SETUP INITIAL BYTECODE SIZE
    viua::internals::types::bytecode_size bytes = 0;


    /////////////////////////
    // GET MAIN FUNCTION NAME
    string main_function = get_main_function(functions.names);
    if (((VERBOSE and main_function != "main/1" and main_function != "")
         or DEBUG)
        and not flags.as_lib) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "main function set to: ";
        cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << main_function
             << send_control_seq(ATTR_RESET);
        cout << endl;
    }


    /////////////////////////////////////////
    // CHECK IF MAIN FUNCTION RETURNS A VALUE
    // FIXME: this is just a crude check - it does not acctually checks if these
    // instructions set 0 register this must be better implemented or we will
    // receive "function did not set return register" exceptions at runtime
    bool main_is_defined =
        (find(functions.names.begin(), functions.names.end(), main_function)
         != functions.names.end());
    if (not flags.as_lib and main_is_defined) {
        check_main_function(main_function, functions.tokens.at(main_function));
    }
    if (not main_is_defined and (DEBUG or VERBOSE) and not flags.as_lib) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "main function (";
        cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << main_function
             << send_control_seq(ATTR_RESET);
        cout << ") is not defined, deferring main function check to post-link "
                "phase"
             << endl;
    }


    /////////////////////////////////
    // MAP FUNCTIONS TO ADDRESSES AND
    // MAP BLOCKS TO ADDRESSES AND
    // SET STARTING INSTRUCTION
    viua::internals::types::bytecode_size starting_instruction =
        0;  // the bytecode offset to first
            // executable instruction
    map<string, viua::internals::types::bytecode_size> function_addresses;
    map<string, viua::internals::types::bytecode_size> block_addresses;
    try {
        block_addresses = map_invocable_addresses(starting_instruction, blocks);
        function_addresses =
            map_invocable_addresses(starting_instruction, functions);
        bytes = viua::cg::tools::calculate_bytecode_size2(tokens);
    } catch (const string& e) {
        throw("bytecode size calculation failed: " + e);
    }


    /////////////////////////////////////////////////////////
    // GATHER LINKS, GET THEIR SIZES AND ADJUST BYTECODE SIZE
    vector<string> links = assembler::ce::getlinks(tokens);
    vector<tuple<string,
                 viua::internals::types::bytecode_size,
                 std::unique_ptr<viua::internals::types::byte[]>>>
        linked_libs_bytecode;
    vector<string> linked_function_names;
    vector<string> linked_block_names;
    map<string, vector<viua::internals::types::bytecode_size>>
        linked_libs_jumptables;

    // map of symbol names to name of the module the symbol came from
    map<string, string> symbol_sources;
    for (auto f : functions.names) {
        symbol_sources[f] = filename;
    }

    for (string lnk : commandline_given_links) {
        if (find(links.begin(), links.end(), lnk) == links.end()) {
            links.emplace_back(lnk);
        } else {
            throw("requested to link module '" + lnk + "' more than once");
        }
    }

    // gather all linked function names
    for (string lnk : links) {
        Loader loader(lnk);
        loader.load();

        vector<string> fn_names = loader.get_functions();
        for (string fn : fn_names) {
            if (function_addresses.count(fn)) {
                throw("duplicate symbol '" + fn + "' found when linking '" + lnk
                      + "' (previously found in '" + symbol_sources.at(fn)
                      + "')");
            }
        }

        map<string, viua::internals::types::bytecode_size> fn_addresses =
            loader.get_function_addresses();
        for (string fn : fn_names) {
            function_addresses[fn] = 0;  // for now we just build a list of all
                                         // available functions
            symbol_sources[fn] = lnk;
            linked_function_names.emplace_back(fn);
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "prelinking function ";
                cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << fn
                     << send_control_seq(ATTR_RESET);
                cout << " from module ";
                cout << send_control_seq(COLOR_FG_WHITE) << lnk
                     << send_control_seq(ATTR_RESET);
                cout << endl;
            }
        }
    }


    //////////////////////////////////////////////////////////////
    // EXTEND FUNCTION NAMES VECTOR WITH NAMES OF LINKED FUNCTIONS
    auto local_function_names = functions.names;
    for (string name : linked_function_names) {
        functions.names.emplace_back(name);
    }


    if (not flags.as_lib) {
        // check if our initial guess for main function is correct and
        // detect some main-function-related errors
        vector<string> main_function_found;
        for (auto f : functions.names) {
            if (f == "main/0" or f == "main/1" or f == "main/2") {
                main_function_found.emplace_back(f);
            }
        }
        if (main_function_found.size() > 1) {
            for (auto f : main_function_found) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_CYAN) << "note"
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << f
                     << send_control_seq(ATTR_RESET);
                cout << " function found in module ";
                cout << send_control_seq(COLOR_FG_WHITE) << symbol_sources.at(f)
                     << send_control_seq(ATTR_RESET);
                cout << endl;
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


    viua::internals::types::bytecode_size current_link_offset = bytes;
    for (string lnk : links) {
        if (DEBUG or VERBOSE) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << "message"
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "[loader] linking with: '";
            cout << send_control_seq(COLOR_FG_WHITE) << lnk
                 << send_control_seq(ATTR_RESET);
            cout << "'" << endl;
        }

        Loader loader(lnk);
        loader.load();

        vector<string> fn_names = loader.get_functions();

        vector<viua::internals::types::bytecode_size> lib_jumps =
            loader.get_jumps();
        if (DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "[loader] entries in jump table: " << lib_jumps.size()
                 << endl;
            for (decltype(lib_jumps)::size_type i = 0; i < lib_jumps.size();
                 ++i) {
                cout << "  jump at byte: " << lib_jumps[i] << endl;
            }
        }

        linked_libs_jumptables[lnk] = lib_jumps;

        map<string, viua::internals::types::bytecode_size> fn_addresses =
            loader.get_function_addresses();
        for (string fn : fn_names) {
            function_addresses[fn] = fn_addresses.at(fn) + current_link_offset;
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "\"" << send_control_seq(COLOR_FG_LIGHT_GREEN) << fn
                     << send_control_seq(ATTR_RESET) << "\": ";
                cout << "entry point at byte: " << current_link_offset << '+'
                     << fn_addresses.at(fn);
                cout << endl;
            }
        }

        linked_libs_bytecode.emplace_back(
            lnk, loader.get_bytecode_size(), loader.get_bytecode());
        bytes += loader.get_bytecode_size();
    }


    /////////////////////////////////////////////////////////////////////////
    // AFTER HAVING OBTAINED LINKED NAMES, IT IS POSSIBLE TO VERIFY CALLS AND
    // CALLABLE (FUNCTIONS, CLOSURES, ETC.) CREATIONS
    assembler::verify::function_calls_are_defined(
        tokens, functions.names, functions.signatures);
    assembler::verify::callable_creations(
        tokens, functions.names, functions.signatures);


    /////////////////////////////
    // REPORT TOTAL BYTECODE SIZE
    if ((VERBOSE or DEBUG) and linked_function_names.size() != 0) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "total required bytes: " << bytes << " bytes" << endl;
    }
    if (DEBUG) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "required bytes: " << (bytes - (bytes - current_link_offset))
             << " local, ";
        cout << (bytes - current_link_offset) << " linked";
        cout << endl;
    }


    ///////////////////////////
    // REPORT FIRST INSTRUCTION
    if ((VERBOSE or DEBUG) and not flags.as_lib) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "first instruction pointer: " << starting_instruction << endl;
    }


    ////////////////////
    // CREATE JUMP TABLE
    vector<viua::internals::types::bytecode_size> jump_table;


    /////////////////////////////////////////////////////////
    // GENERATE BYTECODE OF LOCAL FUNCTIONS AND BLOCKS
    //
    // BYTECODE IS GENERATED HERE BUT NOT YET WRITTEN TO FILE
    // THIS MUST BE GENERATED HERE TO OBTAIN FILL JUMP TABLE
    map<string,
        tuple<viua::internals::types::bytecode_size,
              unique_ptr<viua::internals::types::byte[]>>>
        functions_bytecode;
    map<string,
        tuple<viua::internals::types::bytecode_size,
              unique_ptr<viua::internals::types::byte[]>>>
        block_bodies_bytecode;
    viua::internals::types::bytecode_size functions_section_size    = 0;
    viua::internals::types::bytecode_size block_bodies_section_size = 0;

    vector<tuple<viua::internals::types::bytecode_size,
                 viua::internals::types::bytecode_size>>
        jump_positions;

    for (string name : blocks.names) {
        // do not generate bytecode for blocks that were linked
        if (find(linked_block_names.begin(), linked_block_names.end(), name)
            != linked_block_names.end()) {
            continue;
        }

        if (VERBOSE or DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "generating bytecode for block \"";
            cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name
                 << send_control_seq(ATTR_RESET);
            cout << '"';
        }
        viua::internals::types::bytecode_size fun_bytes = 0;
        try {
            fun_bytes = viua::cg::tools::calculate_bytecode_size2(
                blocks.tokens.at(name));
            if (VERBOSE or DEBUG) {
                cout << " (" << fun_bytes << " bytes at byte "
                     << block_bodies_section_size << ')' << endl;
            }
        } catch (const string& e) {
            throw("failed block size count (during pre-assembling): " + e);
        } catch (const std::out_of_range& e) {
            throw("in block '" + name + "': " + e.what());
        }

        Program func(fun_bytes);
        func.setdebug(DEBUG).setscream(SCREAM);
        try {
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "assembling block '";
                cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name
                     << send_control_seq(ATTR_RESET);
                cout << "'\n";
            }
            assemble(func, strip_attributes(blocks.tokens.at(name)));
        } catch (const string& e) {
            throw("in block '" + name + "': " + e);
        } catch (const char*& e) {
            throw("in block '" + name + "': " + e);
        } catch (const std::out_of_range& e) {
            throw("in block '" + name + "': " + e.what());
        }

        vector<viua::internals::types::bytecode_size> jumps = func.jumps();

        vector<tuple<viua::internals::types::bytecode_size,
                     viua::internals::types::bytecode_size>>
            local_jumps;
        for (auto jmp : jumps) {
            local_jumps.emplace_back(jmp, block_bodies_section_size);
        }
        func.calculate_jumps(local_jumps, blocks.tokens.at(name));

        auto btcode = func.bytecode();

        // store generated bytecode fragment for future use (we must not yet
        // write it to the file to conform to bytecode format)
        block_bodies_bytecode[name] =
            tuple<viua::internals::types::bytecode_size, decltype(btcode)>(
                func.size(), std::move(btcode));

        // extend jump table with jumps from current block
        for (decltype(jumps)::size_type i = 0; i < jumps.size(); ++i) {
            viua::internals::types::bytecode_size jmp = jumps[i];
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "pushed relative jump to jump table: " << jmp << '+'
                     << block_bodies_section_size << endl;
            }
            jump_table.emplace_back(jmp + block_bodies_section_size);
        }

        block_bodies_section_size += func.size();
    }

    // functions section size, must be offset by the size of block section
    functions_section_size = block_bodies_section_size;

    for (string name : functions.names) {
        // do not generate bytecode for functions that were linked
        if (find(linked_function_names.begin(),
                 linked_function_names.end(),
                 name)
            != linked_function_names.end()) {
            continue;
        }

        if (VERBOSE or DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "generating bytecode for function \"";
            cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name
                 << send_control_seq(ATTR_RESET);
            cout << '"';
        }
        viua::internals::types::bytecode_size fun_bytes = 0;
        try {
            fun_bytes = viua::cg::tools::calculate_bytecode_size2(
                functions.tokens.at(name));
            if (VERBOSE or DEBUG) {
                cout << " (" << fun_bytes << " bytes at byte "
                     << functions_section_size << ')' << endl;
            }
        } catch (const string& e) {
            throw("failed function size count (during pre-assembling): " + e);
        } catch (const std::out_of_range& e) {
            throw e.what();
        }

        Program func(fun_bytes);
        func.setdebug(DEBUG).setscream(SCREAM);
        try {
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "assembling function '";
                cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name
                     << send_control_seq(ATTR_RESET);
                cout << "'\n";
            }
            assemble(func, strip_attributes(functions.tokens.at(name)));
        } catch (const string& e) {
            string msg =
                ("in function '" + send_control_seq(COLOR_FG_LIGHT_GREEN) + name
                 + send_control_seq(ATTR_RESET) + "': " + e);
            throw msg;
        } catch (const char*& e) {
            string msg =
                ("in function '" + send_control_seq(COLOR_FG_LIGHT_GREEN) + name
                 + send_control_seq(ATTR_RESET) + "': " + e);
            throw msg;
        } catch (const std::out_of_range& e) {
            string msg =
                ("in function '" + send_control_seq(COLOR_FG_LIGHT_GREEN) + name
                 + send_control_seq(ATTR_RESET) + "': " + e.what());
            throw msg;
        }

        vector<viua::internals::types::bytecode_size> jumps = func.jumps();

        vector<tuple<viua::internals::types::bytecode_size,
                     viua::internals::types::bytecode_size>>
            local_jumps;
        for (decltype(jumps)::size_type i = 0; i < jumps.size(); ++i) {
            viua::internals::types::bytecode_size jmp = jumps[i];
            local_jumps.emplace_back(jmp, functions_section_size);
        }
        func.calculate_jumps(local_jumps, functions.tokens.at(name));

        auto btcode = func.bytecode();

        // store generated bytecode fragment for future use (we must not yet
        // write it to the file to conform to bytecode format)
        functions_bytecode[name] =
            tuple<viua::internals::types::bytecode_size, decltype(btcode)>{
                func.size(), std::move(btcode)};

        // extend jump table with jumps from current function
        for (decltype(jumps)::size_type i = 0; i < jumps.size(); ++i) {
            viua::internals::types::bytecode_size jmp = jumps[i];
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "pushed relative jump to jump table: " << jmp << '+'
                     << functions_section_size << endl;
            }
            jump_table.emplace_back(jmp + functions_section_size);
        }

        functions_section_size += func.size();
    }


    ////////////////////////////////////////
    // CREATE OFSTREAM TO WRITE BYTECODE OUT
    ofstream out(compilename, ios::out | ios::binary);

    out.write(VIUA_MAGIC_NUMBER, sizeof(char) * 5);
    if (flags.as_lib) {
        out.write(&VIUA_LINKABLE, sizeof(ViuaBinaryType));
    } else {
        out.write(&VIUA_EXECUTABLE, sizeof(ViuaBinaryType));
    }


    /////////////////////////////////////////////////////////////
    // WRITE META-INFORMATION MAP
    auto meta_information_map = gather_meta_information(tokens);
    viua::internals::types::bytecode_size meta_information_map_size = 0;
    for (auto each : meta_information_map) {
        meta_information_map_size +=
            (each.first.size() + each.second.size() + 2);
    }

    bwrite(out, meta_information_map_size);
    if (DEBUG) {
        cout << send_control_seq(COLOR_FG_WHITE) << filename
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
             << send_control_seq(ATTR_RESET);
        cout << ": ";
        cout << "writing meta information\n";
    }
    for (auto each : meta_information_map) {
        if (DEBUG) {
            cout << "  " << str::enquote(each.first) << ": "
                 << str::enquote(each.second) << endl;
        }
        strwrite(out, each.first);
        strwrite(out, each.second);
    }


    //////////////////////////
    // IF ASSEMBLING A LIBRARY
    // WRITE OUT JUMP TABLE
    if (flags.as_lib) {
        if (DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "jump table has " << jump_table.size() << " entries"
                 << endl;
        }
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
    viua::internals::types::bytecode_size signatures_section_size = 0;
    for (const auto each : functions.signatures) {
        signatures_section_size += (each.size() + 1);  // +1 for null byte after
                                                       // each signature
    }
    bwrite(out, signatures_section_size);
    for (const auto each : functions.signatures) {
        strwrite(out, each);
    }


    /////////////////////////////////////////////////////////////
    // WRITE EXTERNAL BLOCK SIGNATURES
    signatures_section_size = 0;
    for (const auto each : blocks.signatures) {
        signatures_section_size += (each.size() + 1);  // +1 for null byte after
                                                       // each signature
    }
    bwrite(out, signatures_section_size);
    for (const auto each : blocks.signatures) {
        strwrite(out, each);
    }


    /////////////////////////////////////////////////////////////
    // WRITE BLOCK AND FUNCTION ENTRY POINT ADDRESSES TO BYTECODE
    viua::internals::types::bytecode_size functions_size_so_far =
        write_code_blocks_section(out, blocks, linked_block_names);
    write_code_blocks_section(
        out, functions, linked_function_names, functions_size_so_far);
    for (string name : linked_function_names) {
        strwrite(out, name);
        // mapped address must come after name
        viua::internals::types::bytecode_size address =
            function_addresses[name];
        bwrite(out, address);
    }


    //////////////////////
    // WRITE BYTECODE SIZE
    bwrite(out, bytes);

    auto program_bytecode = make_unique<viua::internals::types::byte[]>(bytes);
    viua::internals::types::bytecode_size program_bytecode_used = 0;

    ////////////////////////////////////////////////////
    // WRITE BYTECODE OF LOCAL BLOCKS TO BYTECODE BUFFER
    for (string name : blocks.names) {
        // linked blocks are to be inserted later
        if (find(linked_block_names.begin(), linked_block_names.end(), name)
            != linked_block_names.end()) {
            continue;
        }

        if (DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "pushing bytecode of local block '";
            cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name
                 << send_control_seq(ATTR_RESET);
            cout << "' to final byte array" << endl;
        }
        viua::internals::types::bytecode_size fun_size =
            get<0>(block_bodies_bytecode[name]);
        viua::internals::types::byte* fun_bytecode =
            get<1>(block_bodies_bytecode[name]).get();

        for (viua::internals::types::bytecode_size i = 0; i < fun_size; ++i) {
            program_bytecode[program_bytecode_used + i] = fun_bytecode[i];
        }
        program_bytecode_used += fun_size;
    }


    ///////////////////////////////////////////////////////
    // WRITE BYTECODE OF LOCAL FUNCTIONS TO BYTECODE BUFFER
    for (string name : functions.names) {
        // linked functions are to be inserted later
        if (find(linked_function_names.begin(),
                 linked_function_names.end(),
                 name)
            != linked_function_names.end()) {
            continue;
        }

        if (DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "pushing bytecode of local function '";
            cout << send_control_seq(COLOR_FG_LIGHT_GREEN) << name
                 << send_control_seq(ATTR_RESET);
            cout << "' to final byte array" << endl;
        }
        viua::internals::types::bytecode_size fun_size =
            get<0>(functions_bytecode[name]);
        viua::internals::types::byte* fun_bytecode =
            get<1>(functions_bytecode[name]).get();

        for (viua::internals::types::bytecode_size i = 0; i < fun_size; ++i) {
            program_bytecode[program_bytecode_used + i] = fun_bytecode[i];
        }
        program_bytecode_used += fun_size;
    }

    ////////////////////////////////////
    // WRITE STATICALLY LINKED LIBRARIES
    viua::internals::types::bytecode_size bytes_offset = current_link_offset;
    for (auto& lnk : linked_libs_bytecode) {
        string lib_name                                   = get<0>(lnk);
        viua::internals::types::byte* linked_bytecode     = get<2>(lnk).get();
        viua::internals::types::bytecode_size linked_size = get<1>(lnk);

        // tie(lib_name, linked_size, linked_bytecode) = lnk;

        if (VERBOSE or DEBUG) {
            cout << send_control_seq(COLOR_FG_WHITE) << filename
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                 << send_control_seq(ATTR_RESET);
            cout << ": ";
            cout << "linked module \"";
            cout << send_control_seq(COLOR_FG_WHITE) << lib_name
                 << send_control_seq(ATTR_RESET);
            cout << "\" written at offset " << bytes_offset << endl;
        }

        vector<viua::internals::types::bytecode_size> linked_jumptable;
        try {
            linked_jumptable = linked_libs_jumptables[lib_name];
        } catch (const std::out_of_range& e) {
            throw("[linker] could not find jumptable for '" + lib_name
                  + "' (maybe not loaded?)");
        }

        viua::internals::types::bytecode_size jmp, jmp_target;
        for (decltype(linked_jumptable)::size_type i = 0;
             i < linked_jumptable.size();
             ++i) {
            jmp                      = linked_jumptable[i];
            aligned_read(jmp_target) = (linked_bytecode + jmp);
            if (DEBUG) {
                cout << send_control_seq(COLOR_FG_WHITE) << filename
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << send_control_seq(COLOR_FG_YELLOW) << "debug"
                     << send_control_seq(ATTR_RESET);
                cout << ": ";
                cout << "adjusting jump: at position " << jmp << ", "
                     << jmp_target << '+' << bytes_offset << " -> "
                     << (jmp_target + bytes_offset) << endl;
            }
            aligned_write(linked_bytecode + jmp) += bytes_offset;
        }

        for (decltype(linked_size) i = 0; i < linked_size; ++i) {
            program_bytecode[program_bytecode_used + i] = linked_bytecode[i];
        }
        program_bytecode_used += linked_size;
    }

    out.write(reinterpret_cast<const char*>(program_bytecode.get()),
              static_cast<std::streamsize>(bytes));
    out.close();
}
