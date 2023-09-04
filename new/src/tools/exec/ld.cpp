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
#include <viua/libs/stage.h>
#include <viua/support/tty.h>
#include <viua/vm/elf.h>

constexpr auto LEFT_QUOTE  = "‘";  // U+2018
constexpr auto RIGHT_QUOTE = "’";  // U+2019

using Text = std::vector<viua::arch::instruction_type>;

namespace stage {
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

namespace {
auto relocate(Text& text, Elf64_Rel const rel, uint64_t const value) -> void
{
    auto const text_ndx = (rel.r_offset / sizeof(viua::arch::instruction_type));

    using viua::arch::ops::F;
    auto hi_op        = F::decode(text.at(text_ndx));
    auto const hi     = static_cast<uint32_t>(value >> 32);
    text.at(text_ndx) = F{hi_op.opcode, hi_op.out, hi}.encode();

    auto lo_op            = F::decode(text.at(text_ndx + 1));
    auto const lo         = static_cast<uint32_t>(value);
    text.at(text_ndx + 1) = F{lo_op.opcode, lo_op.out, lo}.encode();
}

auto is_usable_module(std::filesystem::path const path,
                      viua::vm::elf::Loaded_elf const& module) -> bool
{
    using viua::support::tty::ATTR_RESET;
    using viua::support::tty::COLOR_FG_CYAN;
    using viua::support::tty::COLOR_FG_ORANGE_RED_1;
    using viua::support::tty::COLOR_FG_RED;
    using viua::support::tty::COLOR_FG_RED_1;
    using viua::support::tty::COLOR_FG_WHITE;
    using viua::support::tty::send_escape_seq;
    constexpr auto esc = send_escape_seq;

    if (module.header.e_type != ET_REL) {
        std::cerr << esc(2, COLOR_FG_WHITE) << path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": not an relocatable file\n";
        return false;
    }

    if (auto const f = module.find_fragment(".rodata"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": no strings fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .rodata section found\n";
        return false;
    }
    if (auto const f = module.find_fragment(".symtab"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": no function table fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_CYAN)
                  << "note" << esc(2, ATTR_RESET)
                  << ": no .symtab section found\n";
        return false;
    }
    if (auto const f = module.find_fragment(".strtab"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": no function table fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_CYAN)
                  << "note" << esc(2, ATTR_RESET)
                  << ": no .strtab section found\n";
        return false;
    }
    if (auto const f = module.find_fragment(".text"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": no text fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .text section found\n";
        return false;
    }
    if (auto const f = module.find_fragment(".rel"); not f.has_value()) {
        std::cerr << esc(2, COLOR_FG_WHITE) << path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": no relocation fragment found\n";
        std::cerr << esc(2, COLOR_FG_WHITE) << path.native()
                  << esc(2, ATTR_RESET) << esc(2, COLOR_FG_CYAN) << "note"
                  << esc(2, ATTR_RESET) << ": no .rel section found\n";
        return false;
    }

    return true;
}

auto show_or_anonymous(std::string_view const view) -> std::string_view
{
    return (view.empty() ? "<anonymous>" : view);
}
}  // namespace

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

    auto dump_strtab = false;

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
        } else if (each == "--dump-strtab" or each == "--dump=strtab") {
            dump_strtab = true;
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
            /*
             * Input files start here.
             */
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

    auto entry_addr =
        std::optional<std::pair<uint64_t, std::filesystem::path>>{};

    /*
     * Contents of .text, .rodata, .symtab, etc from all modules are glued
     * together to produce the final ELF.
     */
    auto text   = std::vector<viua::arch::instruction_type>{};
    auto rodata = std::vector<uint8_t>{};
    auto strtab = std::vector<uint8_t>{};
    auto symtab = std::vector<Elf64_Sym>{};

    /*
     * PREPARATIONS FOR .strtab
     *
     * Ensure that the glued-together .strtab begins with a nul character, as
     * ELF mandates. This is needed to conform to the standard and expectations.
     *
     * Also, we HAVE TO allocate all the memory we will need for .strtab RIGHT
     * NOW to avoid reallocations messing up the std::string_view objects that
     * we will be using.
     */
    {
        /*
         * Need space for the beginning and ending nul characters.
         */
        auto needed_strtab_size = size_t{2};

        for (auto const& lnk_path : input_files) {
            auto const lnk_elf_fd = open(lnk_path.c_str(), O_RDONLY);
            if (lnk_elf_fd == -1) {
                auto const saved_errno = errno;
                auto const errname     = strerrorname_np(saved_errno);
                auto const errdesc     = strerrordesc_np(saved_errno);

                std::cerr << esc(2, COLOR_FG_WHITE) << lnk_path.native()
                          << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                          << "error" << esc(2, ATTR_RESET);
                if (errname) {
                    std::cerr << ": " << errname;
                }
                std::cerr << ": " << (errdesc ? errdesc : "unknown error")
                          << "\n";
                return 1;
            }

            using Module    = viua::vm::elf::Loaded_elf;
            auto lnk_module = Module::load(lnk_elf_fd);
            close(lnk_elf_fd);

            if (not is_usable_module(lnk_path, lnk_module)) {
                return 1;
            }

            auto const lnk_strtab_size =
                lnk_module.find_fragment(".strtab")->get().data.size();
            needed_strtab_size += (lnk_strtab_size - 2);
        }
        strtab.reserve(needed_strtab_size);

        if (dump_strtab) {
            std::cerr << "[.strtab] reserved size: " << needed_strtab_size
                      << " bytes\n";
        }
    }
    strtab.push_back('\0');

    /*
     * Ensure that the glued-together .symtab begins with the special empty
     * symbol.
     */
    {
        auto empty     = Elf64_Sym{};
        empty.st_name  = STN_UNDEF;
        empty.st_info  = ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE);
        empty.st_shndx = SHN_UNDEF;
        symtab.push_back(empty);
    }

    /*
     * Map from symbol's name to where it is defined.
     */
    auto symtab_cache =
        std::map<std::string_view, std::pair<size_t, std::filesystem::path>>{};

    /*
     * Map from symbol's location to where it is defined.
     * Used for anonymous symbols.
     */
    auto anonymous_symtab_cache =
        std::map<size_t, std::pair<size_t, std::filesystem::path>>{};

    auto const record_symbol =
        [&symtab, &symtab_cache, &anonymous_symtab_cache](
            std::string_view const name,
            Elf64_Sym const sym,
            std::filesystem::path const path) -> size_t {
        auto const sym_ndx = symtab.size();

        /*
         * Treat anonymous symbols differently. Since they do not have names
         * the linker uses their addresses to differentiate them.
         */
        auto const sym_def = std::pair{symtab.size(), path};
        if (name.empty()) {
            anonymous_symtab_cache.emplace(sym.st_value, sym_def);
        } else {
            symtab_cache.emplace(name, sym_def);
        }
        symtab.push_back(sym);

        return sym_ndx;
    };
    // FIXME also add is_defined(std::string_view) for checks by name
    auto is_defined = [&strtab, &symtab_cache, &anonymous_symtab_cache](
                          Elf64_Sym const sym) -> bool {
        auto const sym_name = std::string_view{
            reinterpret_cast<char const*>(strtab.data()) + sym.st_name};
        return (sym.st_name ? symtab_cache.count(sym_name)
                            : anonymous_symtab_cache.count(sym.st_value));
    };
    auto get_symtab_index = [&strtab, &symtab_cache, &anonymous_symtab_cache](
                                Elf64_Sym const sym) -> size_t {
        auto const sym_name = std::string_view{
            reinterpret_cast<char const*>(strtab.data()) + sym.st_name};
        return (sym_name.empty() ? anonymous_symtab_cache.at(sym.st_value)
                                 : symtab_cache.at(sym_name))
            .first;
    };

    /*
     * Map from symbol's name to its offset in .strtab to make adjusting st_name
     * fields faster.
     */
    auto strtab_cache = std::map<std::string_view, size_t>{};

    /*
     * Map of relocation offsets (Elf64_Rel.r_offset) which were initially
     * pointing to undefined symbols (ie, those which were not defined in the
     * .symtab at the moment they were encountered).
     *
     * It is used to resolve relocations by-name during the global relocation
     * phase. We need to resolve relocations of undefined symbols by-name
     * instead of by-index because we simply DO NOT KNOW at what index they will
     * appear in .symtab, but we DO KNOW their name.
     */
    auto rel_by_name = std::map<size_t, std::string_view>{};

    /*
     * Assume we want to produce an executable - then we need to resolve the
     * relocations in each module.
     */
    auto relocations = std::vector<Elf64_Rel>{};

    for (auto const& lnk_path : input_files) {
        auto const lnk_elf_fd = open(lnk_path.c_str(), O_RDONLY);
        if (lnk_elf_fd == -1) {
            auto const saved_errno = errno;
            auto const errname     = strerrorname_np(saved_errno);
            auto const errdesc     = strerrordesc_np(saved_errno);

            std::cerr << esc(2, COLOR_FG_WHITE) << lnk_path.native()
                      << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                      << "error" << esc(2, ATTR_RESET);
            if (errname) {
                std::cerr << ": " << errname;
            }
            std::cerr << ": " << (errdesc ? errdesc : "unknown error") << "\n";
            return 1;
        }

        using Module    = viua::vm::elf::Loaded_elf;
        auto lnk_module = Module::load(lnk_elf_fd);
        close(lnk_elf_fd);

        if (verbosity_level) {
            std::cerr << "linking: " << lnk_path << "\n";
        }

        auto lnk_text = lnk_module.make_text_from(
            lnk_module.find_fragment(".text")->get().data);
        auto lnk_rodata =
            std::move(lnk_module.find_fragment(".rodata")->get().data);
        auto lnk_symtab = stage::make_symtab_from(
            std::move(lnk_module.find_fragment(".symtab")->get().data));
        auto lnk_strtab =
            std::move(lnk_module.find_fragment(".strtab")->get().data);
        auto lnk_rel = stage::make_relocations_from(
            std::move(lnk_module.find_fragment(".rel")->get().data));

        auto const text_addend =
            (text.size() * sizeof(viua::arch::instruction_type));
        auto const rodata_addend = rodata.size();

        if (auto const ep = lnk_module.entry_point(); ep.has_value()) {
            if (entry_addr.has_value()) {
                std::cerr << esc(2, COLOR_FG_WHITE) << lnk_path.native()
                          << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                          << "error" << esc(2, ATTR_RESET)
                          << ": entry point already defined by "
                          << entry_addr->second.native() << "\n";
                return 1;
            }
            entry_addr = {*ep + text_addend, lnk_path};
        }

        /*
         * This loop adjusts .symtab and .strtab sections.
         * Why both at the same time?
         *
         * Because to adjust .symtab we need to modify values of st_name which
         * is an offset into the .strtab section, and to get the right index we
         * need to know what it will be in the glued-together string table.
         *
         * Se we just re-record the strings and use the new indexes. Simple.
         */
        using viua::libs::stage::save_buffer_to_rodata;
        using viua::libs::stage::save_string_to_strtab;
        auto sym_ndx = size_t{0};
        for (auto& sym : lnk_symtab) {
            auto const lnk_sym_name = std::string_view{
                reinterpret_cast<char const*>(lnk_strtab.data()) + sym.st_name};
            auto const sym_type = ELF64_ST_TYPE(sym.st_info);
            if (verbosity_level) {
                std::cerr << "  " << sym_ndx++ << ": symbol: ";
                switch (sym_type) {
                case STT_NOTYPE:
                    std::cerr << "STT_NOTYPE";
                    break;
                case STT_FUNC:
                    std::cerr << "STT_FUNC";
                    break;
                case STT_OBJECT:
                    std::cerr << "STT_OBJECT";
                    break;
                case STT_FILE:
                    std::cerr << "STT_FILE";
                    break;
                default:
                    std::cerr << "<unknown type>";
                    break;
                }
                std::cerr << ": " << show_or_anonymous(lnk_sym_name) << "\n";
            }

            if (ELF64_ST_TYPE(sym.st_info) == STT_NOTYPE) {
                continue;
            }

            sym.st_name         = save_string_to_strtab(strtab, lnk_sym_name);
            auto const sym_name = std::string_view{
                reinterpret_cast<char const*>(strtab.data()) + sym.st_name};
            if (verbosity_level and sym.st_name) {
                std::cerr << "    global sym name: " << sym_name << "\n";
                std::cerr << "    global .st_name: " << sym.st_name << "\n";
            }

            strtab_cache.emplace(sym_name, sym.st_name);

            /*
             * Just put the name of the file in .symtab and continue. This is a
             * special entry that will tell us from what file the following
             * symbols came, but does not require any further processing.
             */
            if (sym_type == STT_FILE) {
                symtab.push_back(sym);
                continue;
            }

            /*
             * Do not process undefined symbols, and do not put them in the
             * symbol table. This would only pollute the symbol table and
             * require weeding them out later, just complicating the code due to
             * forcing the linker to adjust and patch .symtab indexes over and
             * over again.
             */
            if (not sym.st_value) {
                if (verbosity_level) {
                    std::cerr << "    undefined in this module\n";
                    if (symtab_cache.count(sym_name)) {
                        auto const [def_sym_ndx, def_sym_module] =
                            symtab_cache.at(sym_name);
                        std::cerr << "    defined as symbol " << def_sym_ndx
                                  << " (by module " << def_sym_module.native()
                                  << ")"
                                  << "\n";

                        auto const def_sym = symtab.at(def_sym_ndx);
                        std::cerr << "    address: ";
                        switch (ELF64_ST_TYPE(def_sym.st_info)) {
                        case STT_FUNC:
                            std::cerr << "[.text+0x";
                            break;
                        case STT_OBJECT:
                            std::cerr << "[.rodata+0x";
                            break;
                        default:
                            std::cerr << "[<invalid>+0x";
                            break;
                        }
                        std::cout << std::hex << std::setfill('0')
                                  << std::setw(16) << def_sym.st_value
                                  << std::dec << std::setfill(' ') << "]\n";
                    }
                }
                continue;
            }

            if ((not sym_name.empty()) and symtab_cache.count(sym_name)) {
                auto const [prev_sym_ndx, prev_sym_module] =
                    symtab_cache.at(sym_name);
                std::cerr << esc(2, COLOR_FG_WHITE) << lnk_path.native()
                          << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                          << "error" << esc(2, ATTR_RESET)
                          << ": duplicate definition of symbol " << LEFT_QUOTE
                          << sym_name << RIGHT_QUOTE << "\n";
                std::cerr << esc(2, COLOR_FG_WHITE) << lnk_path.native()
                          << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_CYAN)
                          << "note" << esc(2, ATTR_RESET)
                          << ": previously defined in module "
                          << esc(2, COLOR_FG_WHITE) << prev_sym_module.native()
                          << esc(2, ATTR_RESET) << "\n";
                return 1;
            }

            switch (ELF64_ST_TYPE(sym.st_info)) {
            case STT_FUNC:
                sym.st_value += text_addend;
                break;
            case STT_OBJECT:
                sym.st_value += rodata_addend;
                break;
            }

            auto const sym_ndx = record_symbol(sym_name, sym, lnk_path);
            if (verbosity_level) {
                std::cerr << "    defined as symbol " << sym_ndx << "\n";
                std::cerr << "    address: ";
                switch (ELF64_ST_TYPE(sym.st_info)) {
                case STT_FUNC:
                    std::cerr << "[.text+0x";
                    break;
                case STT_OBJECT:
                    std::cerr << "[.rodata+0x";
                    break;
                default:
                    std::cerr << "[<invalid>+0x";
                    break;
                }
                std::cout << std::hex << std::setfill('0') << std::setw(16)
                          << sym.st_value << std::dec << std::setfill(' ')
                          << "]\n";
            }
        }

        for (auto& rel : lnk_rel) {
            auto const sym_ndx  = ELF64_R_SYM(rel.r_info);
            auto const lnk_sym  = lnk_symtab.at(sym_ndx);
            auto const sym_name = std::string_view{
                reinterpret_cast<char const*>(strtab.data()) + lnk_sym.st_name};

            if (verbosity_level) {
                std::cerr << "  rel at " << rel.r_offset
                          << " for symbol: " << sym_ndx << ": "
                          << show_or_anonymous(sym_name)
                          << " (.st_name = " << lnk_sym.st_name << ")"
                          << "\n";
            }

            if (is_defined(lnk_sym)) {
                auto const patched_ndx = get_symtab_index(lnk_sym);
                if (verbosity_level) {
                    std::cerr << "    defined\n";
                    std::cerr << "    translate .symtab index: " << sym_ndx
                              << " => " << patched_ndx << "\n";
                }

                rel.r_info =
                    ELF64_R_INFO(patched_ndx, ELF64_R_TYPE(rel.r_info));

                /*
                 * Put in the new .symtab index, and leave the final relocation
                 * for later.
                 */
                relocate(lnk_text, rel, patched_ndx);
            } else {
                if (verbosity_level) {
                    std::cerr << "    undefined\n";
                    std::cerr << "    record as by-name relocation at [.text+0x"
                              << std::hex << std::setfill('0') << std::setw(16)
                              << rel.r_offset << std::dec << std::setfill(' ')
                              << "] for " << sym_name << "\n";
                }

                /*
                 * The offset we need to use as key is the one in the final
                 * glued-together .text section. Otherwise there will be a
                 * mismatch between what's in the map and what's in the memory
                 * for all modules except the first one.
                 *
                 * This was caught by the test suite when it used a different
                 * order of ELF modules on the command line, and the offsets got
                 * messed up.
                 */
                rel_by_name.emplace(rel.r_offset + text_addend, sym_name);
            }

            /*
             * Increase the relocation offset to make the final relocation use
             * the correct address. Without this the linker would try to adjust
             * random instructions because the offset into the original .text
             * will not match offset into the glued-together .text section.
             */
            rel.r_offset += text_addend;

            relocations.push_back(rel);
        }

        std::copy(lnk_text.begin(), lnk_text.end(), std::back_inserter(text));

        /*
         * FIXME What about "extern" objects?
         */
        std::copy(
            lnk_rodata.begin(), lnk_rodata.end(), std::back_inserter(rodata));
    }

    /*
     * Ensure that the glued-together .strtab ends with the special case nul
     * terminator, as mandated by ELF.
     */
    strtab.push_back('\0');

    if (verbosity_level) {
        std::cerr << "applying relocations (" << relocations.size() << ")\n";
    }
    auto rel_i = size_t{0};
    for (auto const& rel : relocations) {
        if (verbosity_level) {
            std::cerr << "  " << rel_i++ << ": relocation at [.text+0x" << std::hex
                      << std::setfill('0') << std::setw(16) << rel.r_offset
                      << std::dec << std::setfill(' ') << "]"
                      << "\n";
        }

        auto invalid_relocation = false;

        if (rel_by_name.count(rel.r_offset)) {
            auto const sym_name = rel_by_name.at(rel.r_offset);
            if (not symtab_cache.count(sym_name)) {
                std::cerr << esc(2, COLOR_FG_WHITE) << output_path.native()
                          << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                          << "error" << esc(2, ATTR_RESET)
                          << ": undefined reference to symbol " << LEFT_QUOTE
                          << sym_name << RIGHT_QUOTE << "\n";
                return 1;
            }
            auto const sym_ndx = symtab_cache.at(sym_name).first;
            auto const sym     = symtab.at(sym_ndx);

            if (verbosity_level) {
                std::cerr << "    symbol: " << show_or_anonymous(sym_name)
                          << "\n";
                std::cerr << "    rel-kind: by-name\n";

                std::cerr << "    .st_value: ";
                switch (ELF64_ST_TYPE(sym.st_info)) {
                case STT_FUNC:
                    std::cerr << "[.text+0x";
                    break;
                case STT_OBJECT:
                    std::cerr << "[.rodata+0x";
                    break;
                default:
                    std::cerr << "[<invalid>+0x";
                    invalid_relocation = true;
                    break;
                }
                std::cout << std::hex << std::setfill('0') << std::setw(16)
                          << sym.st_value << std::dec << std::setfill(' ')
                          << "]\n";
            }

            if (invalid_relocation) {
                abort();
            }
            relocate(text, rel, sym.st_value);
        } else {
            auto const sym_ndx  = ELF64_R_SYM(rel.r_info);
            auto const sym      = symtab.at(sym_ndx);
            auto const sym_name = std::string_view{
                reinterpret_cast<char const*>(strtab.data()) + sym.st_name};

            if (verbosity_level) {
                std::cerr << "    symbol: " << show_or_anonymous(sym_name)
                          << "\n";
                std::cerr << "    rel-kind: by-index\n";

                std::cerr << "    .st_value: ";
                switch (ELF64_ST_TYPE(sym.st_info)) {
                case STT_FUNC:
                    std::cerr << "[.text+0x";
                    break;
                case STT_OBJECT:
                    std::cerr << "[.rodata+0x";
                    break;
                default:
                    std::cerr << "[<invalid>+0x";
                    invalid_relocation = true;
                    break;
                }
                std::cout << std::hex << std::setfill('0') << std::setw(16)
                          << sym.st_value << std::dec << std::setfill(' ')
                          << "]\n";
            }

            if (invalid_relocation) {
                abort();
            }
            relocate(text, rel, sym.st_value);
        }
    }

    if ((not entry_addr.has_value()) and as_executable) {
        std::cerr << esc(2, COLOR_FG_WHITE) << output_path.native()
                  << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED)
                  << "error" << esc(2, ATTR_RESET)
                  << ": no entry point defined, but requested output is an "
                     "executable\n";
        return 1;
    }
    if (entry_addr.has_value() and not as_executable) {
        std::cerr
            << esc(2, COLOR_FG_WHITE) << output_path.native()
            << esc(2, ATTR_RESET) << ": " << esc(2, COLOR_FG_RED) << "error"
            << esc(2, ATTR_RESET)
            << ": entry point defined, but requested output is a library\n";
        return 1;
    }

    if (dump_strtab) {
        std::cerr << "[.strtab] allocated size: " << strtab.size()
                  << " bytes\n";
        for (auto i = size_t{0}; i < strtab.size(); ++i) {
            auto const sv = std::string_view{
                reinterpret_cast<char const*>(strtab.data() + i)};
            std::cout << "[.strtab+0x" << std::hex << std::setfill('0')
                      << std::setw(16) << i << std::dec << std::setfill(' ')
                      << "] = " << LEFT_QUOTE << sv << RIGHT_QUOTE << "\n";
            i += sv.size();
        }
    }

    stage::emit_elf(
        output_path,
        as_executable,
        (entry_addr.has_value() ? std::optional{entry_addr->first}
                                : std::nullopt),
        text,
        (as_executable ? std::nullopt : std::optional{std::move(relocations)}),
        rodata,
        strtab,
        symtab);

    return 0;
}
