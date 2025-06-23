/*
 *  Copyright (C) 2023-2025 Marek Marecki
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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <blake2.h>
#include <blake3.h>
#include <sha1.h>
#include <sha2.h>
#include <uuid/uuid.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <optional>
#include <print>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/arch/elf.h>
#include <viua/arch/ops.h>
#include <viua/libexec/common.hh>
#include <viua/libs/stage.h>
#include <viua/support/elf.hh>
#include <viua/support/errno.h>
#include <viua/support/fdio.h>
#include <viua/support/print.hh>
#include <viua/support/string.h>
#include <viua/support/tty.h>
#include <viua/vm/elf.h>


using Text = std::vector<viua::arch::instruction_type>;

using viua::support::string::quote_fancy;

enum class Build_id_hash
{
    UUID,
    SHA1,
    SHA256,
    SHA384,
    SHA512,
    BLAKE2B,
    BLAKE3,
};

namespace stage {
auto emit_elf(
    std::filesystem::path const output_path,
    bool const as_executable,
    std::optional<uint64_t> const entry_point_fn,
    Text const& text,
    std::optional<std::vector<Elf64_Rel>> relocs,
    std::vector<uint8_t> const& rodata_buf,
    std::vector<uint8_t> const& string_table,
    std::vector<Elf64_Sym>& symbol_table,
    std::optional<std::string> const interpreter     = std::nullopt,
    std::optional<Build_id_hash> const build_id_hash = std::nullopt,
    std::optional<size_t> const build_id_size        = std::nullopt) -> void
{
    auto output_buffer = std::vector<uint8_t>{};
    auto const save    = [&output_buffer](
                          void const* const data, size_t const size)
    {
        auto const tail = output_buffer.size();
        output_buffer.resize(tail + size);
        memcpy(output_buffer.data() + tail, data, size);
    };

    auto const a_out = open(output_path.c_str(),
                            O_CREAT | O_TRUNC | O_WRONLY,
                            S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (a_out == -1) {
        close(a_out);
        exit(1);
    }

    using viua::arch::elf::VIUA_MAGIC;
    auto const DEFAULT_VIUA_INTERP =
        std::string{ INSTALL_PREFIX "/libexec/viua/vm" };
    auto const VIUA_INTERP  = interpreter.value_or(DEFAULT_VIUA_INTERP);
    auto const VIUA_COMMENT = std::string{ VIUAVM_VERSION_FULL };

    constexpr auto gnu_namespace = std::string_view{ "GNU", 4 };
    auto build_id                = std::vector<uint8_t>{};
    if (build_id_hash.has_value()) {
        switch (*build_id_hash) {
            using enum Build_id_hash;
            case UUID:
                /*
                 * See uuid_generate(3) for more information, and to learn why
                 * 16 is used here.
                 */
                build_id.resize(16);
                break;
            case SHA1:
                build_id.resize(SHA1_DIGEST_LENGTH);
                break;
            case SHA256:
                build_id.resize(SHA256_DIGEST_LENGTH);
                break;
            case SHA384:
                build_id.resize(SHA384_DIGEST_LENGTH);
                break;
            case SHA512:
                build_id.resize(SHA512_DIGEST_LENGTH);
                break;
            case BLAKE2B:
                build_id.resize(build_id_size.value_or(BLAKE2B_OUTBYTES * 8)
                                / 8);
                break;
            case BLAKE3:
                build_id.resize(build_id_size.value_or(BLAKE3_OUT_LEN * 8) / 8);
                break;
        }

        /*
         * Just in case, reserve the space for the nul terminator required by
         * some algorithms eg, UUID.
         */
        build_id.reserve(build_id.size() + 1);
    }
    auto note_gnu_build_id     = Elf64_Nhdr{};
    note_gnu_build_id.n_namesz = gnu_namespace.size();
    note_gnu_build_id.n_descsz = build_id.size();
    note_gnu_build_id.n_type   = NT_GNU_BUILD_ID;

    auto build_id_offset               = size_t{ 0 };
    auto note_gnu_build_id_section_ndx = size_t{ 0 };
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
        elf_header.e_machine              = EM_VIUAVM;
        elf_header.e_version              = elf_header.e_ident[EI_VERSION];
        elf_header.e_flags  = 0;  // processor-specific flags, should be 0
        elf_header.e_ehsize = sizeof(elf_header);

        auto shstr            = std::vector<char>{ '\0' };
        auto save_shstr_entry = [&shstr](std::string_view const sv) -> size_t
        {
            auto const saved_at = shstr.size();
            std::copy(sv.begin(), sv.end(), std::back_inserter(shstr));
            shstr.push_back('\0');
            return saved_at;
        };

        auto text_section_ndx   = size_t{ 0 };
        auto rel_section_ndx    = size_t{ 0 };
        auto rodata_section_ndx = size_t{ 0 };
        auto symtab_section_ndx = size_t{ 0 };
        auto strtab_section_ndx = size_t{ 0 };

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

            elf_headers.push_back({ seg, void_section });
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
            seg.p_offset = sizeof(Elf64_Ehdr) + offsetof(Elf64_Phdr, p_paddr);
            memcpy(&seg.p_paddr, VIUA_MAGIC.data(), VIUA_MAGIC.size());
            seg.p_filesz = VIUA_MAGIC.size();

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".viua.magic");
            sec.sh_type   = SHT_NOBITS;
            sec.sh_offset = sizeof(Elf64_Ehdr) + offsetof(Elf64_Phdr, p_paddr);
            sec.sh_size   = VIUA_MAGIC.size();
            sec.sh_flags  = 0;

            elf_headers.push_back({ seg, sec });
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
            seg.p_filesz = VIUA_INTERP.size() + 1;
            seg.p_flags  = PF_R;

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".interp");
            sec.sh_type   = SHT_PROGBITS;
            sec.sh_offset = 0;
            sec.sh_size   = VIUA_INTERP.size() + 1;
            sec.sh_flags  = 0;

            elf_headers.push_back({ seg, sec });
        }
        if (build_id_hash.has_value()) {
            /*
             * .note.gnu.build-id
             *
             * The unique bitstring identifying the build.
             */
            Elf64_Phdr seg{};
            seg.p_type   = PT_NOTE;
            seg.p_offset = 0;
            seg.p_filesz = sizeof(note_gnu_build_id) + gnu_namespace.size()
                           + build_id.size();

            Elf64_Shdr sec{};
            sec.sh_name   = save_shstr_entry(".note.gnu.build-id");
            sec.sh_type   = SHT_NOTE;
            sec.sh_offset = 0;
            /*
             * Store the size of the data in the sh_size field so it can be
             * retrieved during global offset calculation.
             *
             * Why do we need this? Because note headers and values must have a
             * 4 byte alignment (see elf(5) discussion of Elf64_Nhdr). At this
             * point we cannot know if this requirement is fulfilled, but after
             * we know sizes of all the sections it is easy to determine.
             *
             * So we just have to wait until we have all the information. Be
             * patient. (It is said to be a virute.)
             */
            sec.sh_size  = seg.p_filesz;
            sec.sh_flags = SHF_ALLOC;

            note_gnu_build_id_section_ndx = elf_headers.size();
            elf_headers.push_back({ seg, sec });
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
            elf_headers.push_back({ std::nullopt, sec });
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
            elf_headers.push_back({ seg, sec });
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
            elf_headers.push_back({ seg, sec });
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

            elf_headers.push_back({ std::nullopt, sec });
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
            elf_headers.push_back({ std::nullopt, sec });
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
            elf_headers.push_back({ std::nullopt, sec });
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

            elf_headers.push_back({ std::nullopt, sec });
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

        auto elf_pheaders = std::count_if(elf_headers.begin(),
                                          elf_headers.end(),
                                          [](auto const& each) -> bool
                                          { return each.first.has_value(); });
        auto elf_sheaders = elf_headers.size();

        auto const elf_size = sizeof(Elf64_Ehdr)
                              + (elf_pheaders * sizeof(Elf64_Phdr))
                              + (elf_sheaders * sizeof(Elf64_Shdr));
        auto text_offset = std::optional<size_t>{};
        {
            auto offset_accumulator = size_t{ 0 };
            for (auto& [segment, section] : elf_headers) {
                if (segment.has_value() and (segment->p_type != PT_NULL)) {
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

                /*
                 * Store the actual offset of the section before any extra
                 * post-processing is applied.
                 *
                 * Why do this here? Because some section types have offset
                 * alignment requirements and without the offset we would not be
                 * able to fulfill them.
                 */
                section.sh_offset = (elf_size + offset_accumulator);

                offset_accumulator += section.sh_size;
            }
        }

        elf_header.e_entry = entry_point_fn.has_value()
                                 ? (*text_offset + *entry_point_fn + elf_size)
                                 : 0;

        elf_header.e_phoff     = sizeof(Elf64_Ehdr);
        elf_header.e_phentsize = sizeof(Elf64_Phdr);
        elf_header.e_phnum     = static_cast<Elf64_Half>(elf_pheaders);

        elf_header.e_shoff =
            elf_header.e_phoff + (elf_pheaders * sizeof(Elf64_Phdr));
        elf_header.e_shentsize = sizeof(Elf64_Shdr);
        elf_header.e_shnum     = static_cast<Elf64_Half>(elf_sheaders);
        elf_header.e_shstrndx  = static_cast<Elf64_Half>(elf_sheaders - 1);

        save(&elf_header, sizeof(elf_header));

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
            save(
                &*segment, sizeof(std::remove_reference_t<decltype(*segment)>));
        }
        for (auto const& [_, section] : elf_headers) {
            save(&section, sizeof(std::remove_reference_t<decltype(section)>));
        }

        save(VIUA_INTERP.c_str(), VIUA_INTERP.size() + 1);

        if (build_id_hash.has_value()) {
            /*
             * .note.gnu.build-id
             */
            save(&note_gnu_build_id, sizeof(note_gnu_build_id));
            save(gnu_namespace.data(), gnu_namespace.size());
            save(build_id.data(), build_id.size());
        }

        if (relocs.has_value()) {
            for (auto const& rel : *relocs) {
                save(&rel, sizeof(std::decay_t<decltype(rel)>));
            }
        }

        auto const text_size =
            (text.size() * sizeof(std::decay_t<decltype(text)>::value_type));
        save(text.data(), text_size);

        save(rodata_buf.data(), rodata_buf.size());

        save(VIUA_COMMENT.c_str(), VIUA_COMMENT.size() + 1);

        for (auto& each : symbol_table) {
            switch (ELF64_ST_TYPE(each.st_info)) {
                case STT_FUNC:
                    each.st_shndx =
                        static_cast<Elf64_Section>(text_section_ndx);
                    break;
                case STT_OBJECT:
                    each.st_shndx =
                        static_cast<Elf64_Section>(rodata_section_ndx);
                    break;
                default:
                    break;
            }
            save(&each, sizeof(std::decay_t<decltype(symbol_table)>));
        }

        save(string_table.data(), string_table.size());

        save(shstr.data(), shstr.size());

        if (build_id_hash.has_value()) {
            build_id_offset =
                elf_headers.at(note_gnu_build_id_section_ndx).second.sh_offset
                + sizeof(Elf64_Nhdr) + gnu_namespace.size();
        }
    }

    if (build_id_hash.has_value()) {
        switch (*build_id_hash) {
            using enum Build_id_hash;
            case UUID:
                {
                    uuid_t uu;
                    uuid_generate_random(uu);
                    std::array<char, 36 + 1> hr{};
                    uuid_unparse(uu, hr.data());
                    memcpy(build_id.data(), &uu, sizeof(uu));
                    break;
                }
            case SHA1:
                {
                    SHA1_CTX context;
                    SHA1Init(&context);
                    SHA1Update(
                        &context, output_buffer.data(), output_buffer.size());
                    SHA1Final(build_id.data(), &context);
                    break;
                }
            case SHA256:
                {
                    SHA2_CTX context;
                    SHA256Init(&context);
                    SHA256Update(
                        &context, output_buffer.data(), output_buffer.size());
                    SHA256Final(build_id.data(), &context);
                    break;
                }
            case SHA384:
                {
                    SHA2_CTX context;
                    SHA384Init(&context);
                    SHA384Update(
                        &context, output_buffer.data(), output_buffer.size());
                    SHA384Final(build_id.data(), &context);
                    break;
                }
            case SHA512:
                {
                    SHA2_CTX context;
                    SHA512Init(&context);
                    SHA512Update(
                        &context, output_buffer.data(), output_buffer.size());
                    SHA512Final(build_id.data(), &context);
                    break;
                }
            case BLAKE2B:
                {
                    blake2b(build_id.data(),
                            output_buffer.data(),
                            nullptr /* key */,
                            build_id.size(),
                            output_buffer.size(),
                            0 /* keylen */);
                    break;
                }
            case BLAKE3:
                {
                    blake3_hasher hasher;
                    blake3_hasher_init(&hasher);
                    blake3_hasher_update(
                        &hasher, output_buffer.data(), output_buffer.size());
                    blake3_hasher_finalize(
                        &hasher, build_id.data(), build_id.size());
                    break;
                }
        }

        memcpy(output_buffer.data() + build_id_offset,
               build_id.data(),
               build_id.size());
    }

    viua::support::posix::whole_write(
        a_out, output_buffer.data(), output_buffer.size());
    close(a_out);
}

auto make_relocations_from(
    std::vector<uint8_t> const data) -> std::vector<Elf64_Rel>
{
    auto rels = std::vector<Elf64_Rel>{};
    rels.reserve(data.size() / sizeof(Elf64_Rel));

    for (auto off = size_t{ 0 }; off < data.size(); off += sizeof(Elf64_Rel)) {
        auto each = Elf64_Rel{};
        memcpy(&each, data.data() + off, sizeof(Elf64_Rel));
        rels.push_back(each);
    }

    return rels;
}

auto make_symtab_from(
    std::vector<uint8_t> const data) -> std::vector<Elf64_Sym>
{
    auto rels = std::vector<Elf64_Sym>{};
    rels.reserve(data.size() / sizeof(Elf64_Sym));

    for (auto off = size_t{ 0 }; off < data.size(); off += sizeof(Elf64_Sym)) {
        auto each = Elf64_Sym{};
        memcpy(&each, data.data() + off, sizeof(Elf64_Sym));
        rels.push_back(each);
    }

    return rels;
}
}  // namespace stage

namespace {
auto relocate(
    Text& text,
    Elf64_Rel const rel,
    uint64_t const value) -> void
{
    auto const text_ndx = (rel.r_offset / sizeof(viua::arch::instruction_type));

    using viua::arch::ops::OPCODE;
    auto const op =
        static_cast<OPCODE>(text.at(text_ndx) & viua::arch::ops::OPCODE_MASK);

    if (op == OPCODE::ARODP or op == OPCODE::ATXTP) {
        using viua::arch::ops::E;
        auto imm_op       = E::decode(text.at(text_ndx));
        text.at(text_ndx) = E{ imm_op.opcode, imm_op.out, value }.encode();
    } else {
        using viua::arch::ops::F;
        auto const hi_ndx = text_ndx - 2;
        auto hi_op        = F::decode(text.at(hi_ndx));
        auto const hi     = static_cast<uint32_t>(value >> 32);
        text.at(hi_ndx)   = F{ hi_op.opcode, hi_op.out, hi }.encode();

        auto const lo_ndx = text_ndx - 1;
        auto lo_op        = F::decode(text.at(lo_ndx));
        auto const lo     = static_cast<uint32_t>(value);
        text.at(lo_ndx)   = F{ lo_op.opcode, lo_op.out, lo }.encode();
    }
}

auto is_usable_module(
    std::filesystem::path const path,
    viua::vm::elf::Loaded_elf const& module) -> bool
{
    if (module.header.e_type != ET_REL) {
        viua::support::errorln(path, "not a relocatable file");
        return false;
    }

    if (auto const f = module.find_fragment(".rodata"); not f.has_value()) {
        viua::support::errorln(path, "no strings fragment found");
        viua::support::noteln(path, "no .rodata section found");
        return false;
    }
    if (auto const f = module.find_fragment(".symtab"); not f.has_value()) {
        viua::support::errorln(path, "no function table fragment found");
        viua::support::noteln(path, "no .symtab section found");
        return false;
    }
    if (auto const f = module.find_fragment(".strtab"); not f.has_value()) {
        viua::support::errorln(path, "no strings table fragment found");
        viua::support::noteln(path, "no .strtab section found");
        return false;
    }
    if (auto const f = module.find_fragment(".text"); not f.has_value()) {
        viua::support::errorln(path, "no text fragment found");
        viua::support::noteln(path, "no .text section found");
        return false;
    }
    if (auto const f = module.find_fragment(".rel"); not f.has_value()) {
        viua::support::errorln(path, "no relocation fragment found");
        viua::support::noteln(path, "no .rel section found");
        return false;
    }

    return true;
}

auto show_or_anonymous(
    std::string_view const view) -> std::string_view
{
    return (view.empty() ? "<anonymous>" : view);
}
}  // namespace

auto main(
    int argc,
    char** argv) -> int
{
    using viua::libexec::Args;
    auto const args = viua::libexec::args_or_exit(
        "ld",
        argc,
        argv,
        {
            VIUA_TOOL_COMMON_OPTIONS,
            { { "o", { "out" } }, Args::Kind::Single },
            { { "", { "type" } }, Args::Kind::Single },
            { { "c", { "object" } }, Args::Kind::Switch },
            { { "", { "static" } }, Args::Kind::Switch },
            { { "", { "dump" } }, Args::Kind::Set },
            { { "i", { "interpreter" } }, Args::Kind::Single },
            { { "", { "build-id" } }, Args::Kind::Single },
            { { "", { "build-id-size" } }, Args::Kind::Single },
        });
    if (args.args.empty()) {
        viua::support::errorln("no files to link");
        return 1;
    }
    auto const verbosity = args.get<bool>("verbose").value_or(false);

    auto const preferred_output_path =
        args.get<std::string_view>("out").transform(
            [](auto&& s) { return std::filesystem::path{ std::move(s) }; });
    auto as_executable = true;

    auto const default_output_type_is_object =
        args.get<bool>("object").value_or(false);
    auto const output_type = args.get<std::string_view>("type").value_or(
        default_output_type_is_object ? "object" : "exec");
    auto const interpreter =
        args.get<std::string_view>("interpreter")
            .transform([](auto const v) { return std::string{ v }; });
    auto const build_id_hash =
        args.get<std::string_view>("build-id")
            .and_then(
                [](auto v) -> std::optional<Build_id_hash>
                {
                    if (v == "none") {
                        return std::nullopt;
                    } else if (v == "uuid") {
                        return Build_id_hash::UUID;
                    } else if (v == "sha1") {
                        return Build_id_hash::SHA1;
                    } else if (v == "sha256") {
                        return Build_id_hash::SHA256;
                    } else if (v == "sha384") {
                        return Build_id_hash::SHA384;
                    } else if (v == "sha512") {
                        return Build_id_hash::SHA512;
                    } else if (v == "blake2b") {
                        return Build_id_hash::BLAKE2B;
                    } else if (v == "blake3") {
                        return Build_id_hash::BLAKE3;
                    } else {
                        viua::support::errorln(
                            "invalid style for --build-id: {}", v);
                        exit(1);
                    }
                });
    auto const build_id_size =
        args.get<std::string_view>("build-id-size")
            .and_then(
                [&args, &build_id_hash](auto v) -> std::optional<size_t>
                {
                    if (not build_id_hash.has_value()) {
                        return std::nullopt;
                    }

                    using enum Build_id_hash;
                    auto const tunable_hashes = std::set{
                        BLAKE2B,
                        BLAKE3,
                    };

                    auto const build_id_hash_name =
                        args.get<std::string_view>("build-id").value();

                    if (not tunable_hashes.contains(*build_id_hash)) {
                        viua::support::errorln(
                            "cannot set digest size with a non-tunable "
                            "--build-id: {}",
                            build_id_hash_name);
                        exit(1);
                    }

                    auto const n = std::stoull(std::string{ v });
                    if (n % 8) {
                        viua::support::errorln(
                            "digest size not divisible by 8: {}", v);
                        exit(1);
                    }

                    if (n < 32) {
                        viua::support::errorln(
                            "--build-id-size: digest size cannot be smaller "
                            "than 32 "
                            "bits");
                        exit(1);
                    }

                    if ((build_id_hash == BLAKE2B) and (n > 512)) {
                        viua::support::errorln(
                            "--build-id-size: maximum digest size for {} is "
                            "512 bits",
                            build_id_hash_name);
                        exit(1);
                    }

                    return n;
                });
    auto as_static_lib = (output_type == "static");
    auto as_shared_lib = (output_type == "shared");
    auto as_object_lib = (output_type == "object");
    auto link_static [[maybe_unused]] =
        args.get<bool>("static").value_or(false);
    auto const input_files =
        std::vector<std::filesystem::path>{ args.args.begin(),
                                            args.args.end() };
    auto const dump_what =
        args.get<std::set<std::string_view>>("dump").value_or(
            std::set<std::string_view>{});
    auto const dump_strtab = dump_what.contains("strtab");

    if (as_static_lib or as_shared_lib or as_object_lib) {
        as_executable = false;
    }

    auto const source_path = input_files.front();
    auto const output_path =
        preferred_output_path
            .or_else(
                [as_executable, source_path, as_shared_lib, as_object_lib]()
                    -> std::optional<std::filesystem::path>
                {
                    if (as_executable) {
                        return std::filesystem::path{ "a.out" };
                    }

                    auto o = source_path;
                    o.replace_extension(
                        as_shared_lib ? "so" : (as_object_lib ? "o" : "a"));
                    return o;
                })
            .value();

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
     * Allocate the first 64 bits of .rodata to prevent any local values from
     * having address of 0.
     */
    rodata.resize(8);

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
        auto needed_strtab_size = size_t{ 2 };

        for (auto const& lnk_path : input_files) {
            auto const lnk_elf_fd = open(lnk_path.c_str(), O_RDONLY);
            if (lnk_elf_fd == -1) {
                auto const saved_errno = errno;
                auto const errname     = viua::support::errno_name(saved_errno);
                auto const errdesc     = viua::support::errno_desc(saved_errno);

                viua::support::errorln(lnk_path, "{}: {}", errname, errdesc);
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
            std::println(stderr,
                         "[.strtab] reserved size: {} bytes",
                         needed_strtab_size);
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
            std::filesystem::path const path) -> size_t
    {
        auto const sym_ndx = symtab.size();

        /*
         * Treat anonymous symbols differently. Since they do not have names
         * the linker uses their addresses to differentiate them.
         */
        auto const sym_def = std::pair{ symtab.size(), path };
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
                          Elf64_Sym const sym) -> bool
    {
        auto const sym_name =
            std::string_view{ reinterpret_cast<char const*>(strtab.data())
                              + sym.st_name };
        return (sym.st_name ? symtab_cache.count(sym_name)
                            : anonymous_symtab_cache.count(sym.st_value));
    };
    auto get_symtab_index = [&strtab, &symtab_cache, &anonymous_symtab_cache](
                                Elf64_Sym const sym) -> size_t
    {
        auto const sym_name =
            std::string_view{ reinterpret_cast<char const*>(strtab.data())
                              + sym.st_name };
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
            auto const errname     = viua::support::errno_name(saved_errno);
            auto const errdesc     = viua::support::errno_desc(saved_errno);

            viua::support::errorln(lnk_path, "{}: {}", errname, errdesc);
            return 1;
        }

        using Module    = viua::vm::elf::Loaded_elf;
        auto lnk_module = Module::load(lnk_elf_fd);
        close(lnk_elf_fd);

        if (verbosity) {
            std::println(stderr, "linking: {}", lnk_path.native());
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
                viua::support::errorln(lnk_path,
                                       "entry point already defined by {}",
                                       entry_addr->second.native());
                return 1;
            }
            entry_addr = { *ep + text_addend, lnk_path };
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
        auto sym_ndx = size_t{ 0 };
        for (auto& sym : lnk_symtab) {
            auto const lnk_sym_name = std::string_view{
                reinterpret_cast<char const*>(lnk_strtab.data()) + sym.st_name
            };
            if (verbosity) {
                auto const sym_type_human_readable =
                    viua::st_type_to_string(sym.st_info);
                std::println(stderr,
                             "{}: symbol: {}: {}",
                             sym_ndx++,
                             sym_type_human_readable,
                             show_or_anonymous(lnk_sym_name));
            }

            if (ELF64_ST_TYPE(sym.st_info) == STT_NOTYPE) {
                continue;
            }

            sym.st_name = save_string_to_strtab(strtab, lnk_sym_name);
            auto const sym_name =
                std::string_view{ reinterpret_cast<char const*>(strtab.data())
                                  + sym.st_name };
            if (verbosity) {
                std::println(stderr, "    global sym name: {}", sym_name);
                std::println(stderr, "    global .st_name: {}", sym.st_name);
            }

            strtab_cache.emplace(sym_name, sym.st_name);

            /*
             * Just put the name of the file in .symtab and continue. This is a
             * special entry that will tell us from what file the following
             * symbols came, but does not require any further processing.
             */
            if (ELF64_ST_TYPE(sym.st_info) == STT_FILE) {
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
                if (verbosity) {
                    std::println(stderr, "    undefined in this module");
                    if (symtab_cache.count(sym_name)) {
                        auto const [def_sym_ndx, def_sym_module] =
                            symtab_cache.at(sym_name);
                        std::println(stderr,
                                     "    defined as symbol {} (by module {})",
                                     def_sym_ndx,
                                     def_sym_module.native());

                        auto const def_sym    = symtab.at(def_sym_ndx);
                        auto in_which_section = std::string_view{};
                        switch (ELF64_ST_TYPE(def_sym.st_info)) {
                            case STT_FUNC:
                                in_which_section = "text";
                                break;
                            case STT_OBJECT:
                                in_which_section = "rodata";
                                break;
                            default:
                                in_which_section = "<invalid>";
                                break;
                        }
                        std::println(stderr,
                                     "    address: [.{}+0x{:016x}]",
                                     in_which_section,
                                     def_sym.st_value);
                    }
                }
                continue;
            }

            if ((not sym_name.empty()) and symtab_cache.count(sym_name)) {
                auto const [prev_sym_ndx, prev_sym_module] =
                    symtab_cache.at(sym_name);
                viua::support::errorln(lnk_path,
                                       "duplicate definition of symbol {}",
                                       quote_fancy(sym_name));
                viua::support::noteln(lnk_path,
                                      "previously defined in module {}",
                                      prev_sym_module.native());
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
            if (verbosity) {
                auto in_which_section = std::string_view{};
                switch (ELF64_ST_TYPE(sym.st_info)) {
                    case STT_FUNC:
                        in_which_section = "text";
                        break;
                    case STT_OBJECT:
                        in_which_section = "rodata";
                        break;
                    default:
                        in_which_section = "<invalid>";
                        break;
                }
                std::println(stderr, "    defined as symbol {}", sym_ndx);
                std::println(stderr,
                             "    address: [.{}+0x{:016x}]",
                             in_which_section,
                             sym.st_value);
            }
        }

        for (auto& rel : lnk_rel) {
            auto const sym_ndx = ELF64_R_SYM(rel.r_info);
            auto const lnk_sym = lnk_symtab.at(sym_ndx);
            auto const sym_name =
                std::string_view{ reinterpret_cast<char const*>(strtab.data())
                                  + lnk_sym.st_name };

            if (verbosity) {
                std::println(stderr,
                             "  rel at {} for symbol: {}: (.st_name = {})",
                             rel.r_offset,
                             sym_ndx,
                             show_or_anonymous(sym_name),
                             lnk_sym.st_name);
            }

            if (is_defined(lnk_sym)) {
                auto const patched_ndx = get_symtab_index(lnk_sym);
                if (verbosity) {
                    std::println(stderr, "    defined");
                    std::println(stderr,
                                 "    translate .symtab index: {} => {}",
                                 sym_ndx,
                                 patched_ndx);
                }

                rel.r_info =
                    ELF64_R_INFO(patched_ndx, ELF64_R_TYPE(rel.r_info));

                /*
                 * Put in the new .symtab index, and leave the final relocation
                 * for later.
                 */
                relocate(lnk_text, rel, patched_ndx);
            } else {
                if (verbosity) {
                    std::println(stderr, "    undefined");
                    std::println(stderr,
                                 "    record as by-name relocation at "
                                 "[.text+0x{:016x}] for {}",
                                 rel.r_offset,
                                 sym_name);
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

    if (verbosity) {
        std::println(stderr, "applying {} relocations", relocations.size());
    }
    auto rel_i = size_t{ 0 };
    for (auto const& rel : relocations) {
        if (verbosity) {
            std::println(stderr,
                         "  {}: relocation at [.text+0x{:016x}]",
                         rel_i,
                         rel.r_offset);
            ++rel_i;
        }

        auto invalid_relocation = false;

        if (rel_by_name.count(rel.r_offset)) {
            auto const sym_name = rel_by_name.at(rel.r_offset);
            if (not symtab_cache.count(sym_name)) {
                viua::support::errorln(
                    "undefined reference to symbol {}", quote_fancy(sym_name));
                return 1;
            }
            auto const sym_ndx = symtab_cache.at(sym_name).first;
            auto const sym     = symtab.at(sym_ndx);

            if (verbosity) {
                auto in_which_section = std::string_view{};
                switch (ELF64_ST_TYPE(sym.st_info)) {
                    case STT_FUNC:
                        in_which_section = "text";
                        break;
                    case STT_OBJECT:
                        in_which_section = "rodata";
                        break;
                    default:
                        in_which_section   = "<invalid>";
                        invalid_relocation = true;
                        break;
                }

                std::println(
                    stderr, "    symbol: {}", show_or_anonymous(sym_name));
                std::println(stderr, "    rel-kind: by-name");
                std::println(stderr,
                             "    .st_value: [{}+0x{:016x}]",
                             in_which_section,
                             sym.st_value);
            }

            if (invalid_relocation) {
                abort();
            }
            relocate(text, rel, sym.st_value);
        } else {
            auto const sym_ndx = ELF64_R_SYM(rel.r_info);
            auto const sym     = symtab.at(sym_ndx);
            auto const sym_name =
                std::string_view{ reinterpret_cast<char const*>(strtab.data())
                                  + sym.st_name };

            if (verbosity) {
                auto in_which_section = std::string_view{};
                switch (ELF64_ST_TYPE(sym.st_info)) {
                    case STT_FUNC:
                        in_which_section = "text";
                        break;
                    case STT_OBJECT:
                        in_which_section = "rodata";
                        break;
                    default:
                        in_which_section   = "<invalid>";
                        invalid_relocation = true;
                        break;
                }

                std::println(
                    stderr, "    symbol: {}", show_or_anonymous(sym_name));
                std::println(stderr, "    rel-kind: by-index");
                std::println(stderr,
                             "    .st_value: [{}+0x{:016x}]",
                             in_which_section,
                             sym.st_value);
            }

            if (invalid_relocation) {
                abort();
            }
            relocate(text, rel, sym.st_value);
        }
    }

    if ((not entry_addr.has_value()) and as_executable) {
        viua::support::errorln(
            output_path,
            "no entry point defined, but requested --type {}",
            output_type);
        return 1;
    }
    if (entry_addr.has_value() and not as_executable) {
        viua::support::errorln(output_path,
                               "entry point defined, but requested --type {}",
                               output_type);
        return 1;
    }

    if (dump_strtab) {
        std::println(
            stderr, "[.strtab] allocated size: {} bytes", strtab.size());
        for (auto i = size_t{ 0 }; i < strtab.size(); ++i) {
            auto const sv = std::string_view{ reinterpret_cast<char const*>(
                strtab.data() + i) };

            std::println(
                stderr, "[.strtab+0x{:016x}] = {}", i, quote_fancy(sv));
            i += sv.size();
        }
    }

    stage::emit_elf(output_path,
                    as_executable,
                    (entry_addr.has_value() ? std::optional{ entry_addr->first }
                                            : std::nullopt),
                    text,
                    (as_executable ? std::nullopt
                                   : std::optional{ std::move(relocations) }),
                    rodata,
                    strtab,
                    symtab,
                    interpreter,
                    build_id_hash,
                    build_id_size);

    return 0;
}
