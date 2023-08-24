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

#include <elf.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <optional>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/arch/ops.h>
#include <viua/support/tty.h>
#include <viua/vm/elf.h>


namespace stage {
using Text = std::vector<viua::arch::instruction_type>;
auto emit_elf(std::filesystem::path const output_path,
              bool const as_executable,
              std::optional<uint64_t> const entry_point_fn,
              Text const& text,
              std::optional<std::vector<Elf64_Rel>> relocs,
              std::vector<uint8_t> const& rodata_buf,
              std::vector<uint8_t> const& string_table,
              std::vector<Elf64_Sym>& symbol_table) -> void
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
    auto const VIUA_COMMENT = std::string{VIUAVM_VERSION_FULL};

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

        auto text_section_ndx   = size_t{0};
        auto rel_section_ndx    = size_t{0};
        auto rodata_section_ndx = size_t{0};
        auto symtab_section_ndx = size_t{0};
        auto strtab_section_ndx = size_t{0};

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
            Elf64_Phdr seg{};
            seg.p_type   = PT_NULL;
            seg.p_offset = 0;
            seg.p_filesz = 0;

            Elf64_Shdr void_section{};
            void_section.sh_type = SHT_NULL;

            elf_headers.push_back({seg, void_section});
        }
        {
            /*
             * .viua.magic
             *
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
             * .interp
             *
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
        if (relocs.has_value()) {
            /*
             * .rel
             */
            auto const relocation_table = *relocs;

            Elf64_Shdr sec{};
            sec.sh_name    = save_shstr_entry(".rel");
            sec.sh_type    = SHT_REL;
            sec.sh_offset  = 0;
            sec.sh_entsize = sizeof(decltype(relocation_table)::value_type);
            sec.sh_size    = (relocation_table.size() * sec.sh_entsize);
            sec.sh_flags   = SHF_INFO_LINK;

            /*
             * This should point to .symtab section that is relevant for the
             * relocations contained in this .rel section (in our case its the
             * only .symtab section in the ELF), but we do not know that
             * section's index yet.
             */
            sec.sh_link = 0;

            /*
             * This should point to .text section (or any other section) to
             * which the relocations apply. We do not know that index yet, but
             * it MUST be patched later.
             */
            sec.sh_info = 0;

            rel_section_ndx = elf_headers.size();
            elf_headers.push_back({std::nullopt, sec});
        }
        {
            /*
             * .text
             *
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

            text_section_ndx = elf_headers.size();
            elf_headers.push_back({seg, sec});
        }
        {
            /*
             * .rodata
             *
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
            auto const sz = rodata_buf.size();
            seg.p_filesz = seg.p_memsz = sz;
            seg.p_flags                = PF_R;
            seg.p_align                = sizeof(viua::arch::instruction_type);

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".rodata");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = seg.p_filesz;
            sec.sh_flags  = SHF_ALLOC;

            rodata_section_ndx = elf_headers.size();
            elf_headers.push_back({seg, sec});
        }
        {
            /*
             * .comment
             */
            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".comment");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = VIUA_COMMENT.size() + 1;
            sec.sh_flags  = 0;

            elf_headers.push_back({std::nullopt, sec});
        }
        {
            /*
             * .symtab
             *
             * Last, but not least, comes another LOAD segment.
             * It contains a symbol table with function addresses.
             *
             * Function calls use this table to determine the address to which
             * they should transfer control - there are no direct calls.
             * Inefficient, but flexible.
             */
            Elf64_Shdr sec{};
            sec.sh_name = save_shstr_entry(".symtab");
            /*
             * This could be SHT_SYMTAB, but the SHT_SYMTAB type sections expect
             * a certain format of the symbol table which Viua does not use. So
             * let's just use SHT_PROGBITS because interpretation of
             * SHT_PROGBITS is up to the program.
             */
            sec.sh_type    = SHT_SYMTAB;
            sec.sh_offset  = 0;
            sec.sh_size    = (symbol_table.size() * sizeof(Elf64_Sym));
            sec.sh_flags   = 0;
            sec.sh_entsize = sizeof(Elf64_Sym);
            sec.sh_info    = 0;

            symtab_section_ndx = elf_headers.size();
            elf_headers.push_back({std::nullopt, sec});
        }
        {
            /*
             * .strtab
             */
            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".strtab");
            sec.sh_type   = SHT_STRTAB;
            sec.sh_offset = 0;
            sec.sh_size   = string_table.size();
            sec.sh_flags  = SHF_STRINGS;

            strtab_section_ndx = elf_headers.size();
            elf_headers.push_back({std::nullopt, sec});
        }
        {
            /*
             * .shstrtab
             *
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
            sec.sh_flags  = SHF_STRINGS;

            elf_headers.push_back({std::nullopt, sec});
        }

        /*
         * Link the .symtab to its associated .strtab; otherwise you will
         * get <corrupt> names when invoking readelf(1) to inspect the file.
         */
        elf_headers.at(symtab_section_ndx).second.sh_link = strtab_section_ndx;

        /*
         * Patch the symbol table section index.
         */
        if (relocs.has_value()) {
            elf_headers.at(rel_section_ndx).second.sh_link = symtab_section_ndx;
            elf_headers.at(rel_section_ndx).second.sh_info = text_section_ndx;
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

        elf_header.e_phoff     = sizeof(Elf64_Ehdr);
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

        if (relocs.has_value()) {
            for (auto const& rel : *relocs) {
                write(a_out, &rel, sizeof(std::decay_t<decltype(rel)>));
            }
        }

        auto const text_size =
            (text.size() * sizeof(std::decay_t<decltype(text)>::value_type));
        write(a_out, text.data(), text_size);

        write(a_out, rodata_buf.data(), rodata_buf.size());

        write(a_out, VIUA_COMMENT.c_str(), VIUA_COMMENT.size() + 1);

        for (auto& each : symbol_table) {
            switch (ELF64_ST_TYPE(each.st_info)) {
            case STT_FUNC:
                each.st_shndx = text_section_ndx;
                break;
            case STT_OBJECT:
                each.st_shndx = rodata_section_ndx;
                break;
            default:
                break;
            }
            write(a_out, &each, sizeof(std::decay_t<decltype(symbol_table)>));
        }

        write(a_out, string_table.data(), string_table.size());

        write(a_out, shstr.data(), shstr.size());
    }

    close(a_out);
}

auto make_relocations_from(std::vector<uint8_t> const data)
    -> std::vector<Elf64_Rel>
{
    auto rels = std::vector<Elf64_Rel>{};
    rels.reserve(data.size() / sizeof(Elf64_Rel));

    for (auto off = size_t{0}; off < data.size(); off += sizeof(Elf64_Rel)) {
        auto each = Elf64_Rel{};
        memcpy(&each, data.data() + off, sizeof(Elf64_Rel));
        rels.push_back(each);
    }

    return rels;
}

auto make_symtab_from(std::vector<uint8_t> const data) -> std::vector<Elf64_Sym>
{
    auto rels = std::vector<Elf64_Sym>{};
    rels.reserve(data.size() / sizeof(Elf64_Sym));

    for (auto off = size_t{0}; off < data.size(); off += sizeof(Elf64_Sym)) {
        auto each = Elf64_Sym{};
        memcpy(&each, data.data() + off, sizeof(Elf64_Sym));
        rels.push_back(each);
    }

    return rels;
}
}  // namespace stage


auto main(int argc, char** argv) -> int
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
                  << ": no file to link\n";
        return 1;
    }

    auto preferred_output_path = std::optional<std::filesystem::path>{};
    auto as_executable         = true;
    auto as_static_lib [[maybe_unused]] = false;
    auto as_shared_lib                  = false;
    auto as_object_lib                  = false;
    auto link_static [[maybe_unused]]   = false;
    auto verbosity_level                = 0;
    auto show_version                   = false;
    auto show_help                      = false;
    auto input_files                    = std::vector<std::filesystem::path>{};

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
        } else if (each == "--type=shared") {
            as_shared_lib = true;
        } else if (each == "--type=exec") {
            as_executable = true;
        } else if (each == "--type=static") {
            as_static_lib = true;
        } else if (each == "--type=object" or each == "-c") {
            as_object_lib = true;
        } else if (each == "--static") {
            link_static = true;
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
            std::copy(
                args.begin() + i, args.end(), std::back_inserter(input_files));
            break;
        }
    }

    if (as_static_lib or as_shared_lib or as_object_lib) {
        as_executable = false;
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
        if (execlp("man", "man", "1", "viua-ld", nullptr) == -1) {
            std::cerr << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET)
                      << ": man(1) page not installed or not found\n";
            return 1;
        }
    }

    auto const source_path = input_files.front();
    auto const output_path = preferred_output_path.value_or(
        as_executable ? std::filesystem::path{"a.out"}
                      : [source_path,
                         as_shared_lib,
                         as_object_lib]() -> std::filesystem::path {
            auto o = source_path;
            o.replace_extension(as_shared_lib ? "so"
                                              : (as_object_lib ? "o" : "a"));
            return o;
        }());

    /*
     * Even if the path exists and is a regular file we should check if it was
     * opened correctly.
     */
    auto const elf_fd = open(source_path.c_str(), O_RDONLY);
    if (elf_fd == -1) {
        auto const saved_errno = errno;
        auto const errname     = strerrorname_np(saved_errno);
        auto const errdesc     = strerrordesc_np(saved_errno);

        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_RED) << "error"
                  << esc(2, ATTR_RESET);
        if (errname) {
            std::cerr << ": " << errname;
        }
        std::cerr << ": " << (errdesc ? errdesc : "unknown error") << "\n";
        return 1;
    }

    using Module           = viua::vm::elf::Loaded_elf;
    auto const main_module = [elf_fd, &source_path]() -> Module {
        try {
            return Module::load(elf_fd);
        } catch (std::runtime_error const& e) {
            std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                      << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                      << "error" << esc(2, ATTR_RESET) << ": " << e.what()
                      << "\n";
            exit(1);
        }
    }();

    if (main_module.header.e_type != ET_REL) {
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": not an relocatable file\n";
        return 1;
    }

    if (auto const f = main_module.find_fragment(".rodata");
        not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": no strings fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .rodata section found\n";
        return 1;
    }
    if (auto const f = main_module.find_fragment(".symtab");
        not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": no function table fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_CYAN)
                  << "note" << esc(2, ATTR_RESET)
                  << ": no .symtab section found\n";
        return 1;
    }
    if (auto const f = main_module.find_fragment(".strtab");
        not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": no function table fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_CYAN)
                  << "note" << esc(2, ATTR_RESET)
                  << ": no .strtab section found\n";
        return 1;
    }
    if (auto const f = main_module.find_fragment(".text"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": no text fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .text section found\n";
        return 1;
    }
    if (auto const f = main_module.find_fragment(".rel"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": no relocation fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .rel section found\n";
        return 1;
    }

    auto entry_addr = std::optional<uint64_t>{};
    ;
    if (auto const ep = main_module.entry_point(); ep.has_value()) {
        entry_addr = *ep;
    }
    if ((not entry_addr.has_value()) and as_executable) {
        std::cerr << esc(2, COLOR_FG_WHITE) << source_path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": no entry point defined, but requested output is an "
                     "executable\n";
        return 1;
    }
    if (entry_addr.has_value() and not as_executable) {
        std::cerr
            << esc(2, COLOR_FG_WHITE) << source_path.native()
            << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED) << "error"
            << esc(2, ATTR_RESET)
            << ": entry point defined, but requested output is a library\n";
        return 1;
    }

    /*
     * These variables are the foundations of our output.
     *
     * Contents of .text, .rodata, .symtab, etc from all modules are glued
     * together to produce the final ELF.
     */
    auto text = main_module.make_text_from(
        main_module.find_fragment(".text")->get().data);
    auto rodata = std::move(main_module.find_fragment(".rodata")->get().data);
    auto symtab = stage::make_symtab_from(
        std::move(main_module.find_fragment(".symtab")->get().data));
    auto strtab = std::move(main_module.find_fragment(".strtab")->get().data);

    /*
     * Assume we want to produce an executable - then we need to resolve the
     * relocations in each module.
     */
    auto relocations = stage::make_relocations_from(
        std::move(main_module.find_fragment(".rel")->get().data));
    for (auto const& rel : relocations) {
        auto const sym_ndx = ELF64_R_SYM(rel.r_info);
        auto const sym     = symtab.at(sym_ndx);

        auto const text_ndx =
            (rel.r_offset / sizeof(viua::arch::instruction_type));
        std::cerr << "relocation at " << rel.r_offset
                  << " [index = " << text_ndx << "]"
                  << " to " << sym_ndx << "\n";
        switch (ELF64_ST_TYPE(sym.st_info)) {
        case STT_FUNC:
            std::cerr << "  function\n";
            break;
        case STT_OBJECT:
            std::cerr << "  object\n";
            break;
        default:
            std::cerr << "  not suitable for relocation\n";
            break;
        }
        if (ELF64_ST_TYPE(sym.st_info) != STT_FUNC) {
            continue;
        }

        std::cerr << "  name = " << &strtab.at(sym.st_name) << "\n";
        std::cerr << "  addr = " << sym.st_value << "\n";

        using viua::arch::ops::F;
        auto hi_op        = F::decode(text.at(text_ndx));
        auto const hi     = static_cast<uint32_t>(sym.st_value >> 32);
        text.at(text_ndx) = F{hi_op.opcode, hi_op.out, hi}.encode();

        auto lo_op            = F::decode(text.at(text_ndx + 1));
        auto const lo         = static_cast<uint32_t>(sym.st_value);
        text.at(text_ndx + 1) = F{lo_op.opcode, lo_op.out, lo}.encode();
    }

    stage::emit_elf(
        output_path,
        as_executable,
        entry_addr,
        text,
        (as_executable ? std::nullopt : std::optional{std::move(relocations)}),
        rodata,
        strtab,
        symtab);

    return 0;
}
