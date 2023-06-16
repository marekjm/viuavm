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
#include <viua/libs/stage.h>
#include <viua/support/string.h>
#include <viua/support/tty.h>
#include <viua/support/vector.h>


constexpr auto DEBUG_LEX       = false;
constexpr auto DEBUG_EXPANSION = false;

using viua::libs::stage::save_string;

namespace {
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
            *ip++ = viua::libs::stage::emit_instruction(insn);
        }
    }

    return fn_addresses;
}
}  // namespace

namespace stage {
using Lexemes   = std::vector<viua::libs::lexer::Lexeme>;
using AST_nodes = std::vector<std::unique_ptr<ast::Node>>;
auto syntactical_analysis(std::filesystem::path const source_path,
                          std::string_view const source_text,
                          Lexemes const& lexemes) -> AST_nodes
{
    try {
        return parse(lexemes);
    } catch (viua::libs::errors::compile_time::Error const& e) {
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
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
                        viua::libs::stage::display_error_and_exit(
                            source_path, source_text, e);
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
            auto const s              = ct.value.front().text;
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
            try {
                auto c = viua::libs::stage::cook_long_immediates(insn, strings_table, var_offsets);
                std::copy(c.begin(), c.end(), std::back_inserter(cooked));
            } catch (viua::libs::errors::compile_time::Error const& e) {
                viua::libs::stage::display_error_in_function(
                    source_path, e, fn.name.text);
                viua::libs::stage::display_error_and_exit(
                    source_path, source_text, e);
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
            fn.instructions = viua::libs::stage::expand_pseudoinstructions(
                std::move(fn.instructions), fn_offsets);
        } catch (viua::libs::errors::compile_time::Error const& e) {
            viua::libs::stage::display_error_in_function(
                source_path, e, fn.name.text);
            viua::libs::stage::display_error_and_exit(
                source_path, source_text, e);
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
                viua::libs::stage::display_error_and_exit(
                    source_path, source_text, e);
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

        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET) << ": "
                  << "no entry point function defined\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_CYAN)
                  << "note" << esc(2, ATTR_RESET) << ": "
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
            viua::libs::stage::display_error_in_function(
                source_path, e, *fn_name);
        }
        viua::libs::stage::display_error_and_exit(source_path, source_text, e);
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
    auto show_help             = false;

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
        } else if (each == "--help") {
            show_help = true;
        } else if (each.front() == '-') {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": unknown option: " << each << "\n";
            return 1;
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
    if (show_help) {
        if (execlp("man", "man", "1", "viua-asm", nullptr) == -1) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": man(1) page not installed or not found\n";
            return 1;
        }
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

        struct stat source_stat {};
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
    auto lexemes =
        viua::libs::lexer::stage::lexical_analysis(source_path, source_text);
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
