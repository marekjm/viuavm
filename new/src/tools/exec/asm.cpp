#include <viua/support/tty.h>
#include <viua/support/vector.h>
#include <viua/libs/lexer.h>
#include <viua/arch/arch.h>
#include <viua/arch/ops.h>

#include <iostream>
#include <iomanip>
#include <chrono>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <utility>
#include <thread>
#include <type_traits>

#include <elf.h>
#include <endian.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


auto to_loading_parts_unsigned(uint64_t const value)
    -> std::pair<uint64_t, std::pair<std::pair<uint32_t, uint32_t>, uint32_t>>
{
    constexpr auto LOW_24  = uint64_t{0x0000000000ffffff};
    constexpr auto HIGH_36 = uint64_t{0xfffffffff0000000};

    auto const high_part = ((value & HIGH_36) >> 28);
    auto const low_part = static_cast<uint32_t>(value & ~HIGH_36);

    /*
     * If the low part consists of only 24 bits we can use just two
     * instructions:
     *
     *  1/ lui to load high 36 bits
     *  2/ addi to add low 24 bits
     *
     * This reduces the overhead of loading 64-bit values.
     */
    if ((low_part & LOW_24) == low_part) {
        return { high_part, { { low_part, 0 }, 0 } };
    }

    auto const multiplier = 16;
    auto const remainder = (low_part % multiplier);
    auto const base = (low_part - remainder) / multiplier;

    return { high_part, { { base, multiplier }, remainder } };
}

namespace {
    auto op_li(uint64_t* instructions, uint64_t const value) -> uint64_t*
    {
        auto const parts = to_loading_parts_unsigned(value);

        /*
         * Only use the lui instruction of there's a reason to ie, if some of
         * the highest 36 bits are set. Otherwise, the lui is just overhead.
         */
        if (parts.first) {
            *instructions++ = viua::arch::ops::E{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::LUIU))
                , viua::arch::Register_access::make_local(1)
                , parts.first
            }.encode();
        }

        auto const base = parts.second.first.first;
        auto const multiplier = parts.second.first.second;

        if (multiplier != 0) {
            *instructions++ = viua::arch::ops::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDIU))
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_void()
                , base
            }.encode();
            *instructions++ = viua::arch::ops::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDIU))
                , viua::arch::Register_access::make_local(3)
                , viua::arch::Register_access::make_void()
                , multiplier
            }.encode();
            *instructions++ = viua::arch::ops::T{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::MUL))
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(3)
            }.encode();

            auto const remainder = parts.second.second;
            *instructions++ = viua::arch::ops::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDIU))
                , viua::arch::Register_access::make_local(3)
                , viua::arch::Register_access::make_void()
                , remainder
            }.encode();
            *instructions++ = viua::arch::ops::T{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADD))
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(3)
            }.encode();

            *instructions++ = viua::arch::ops::T{
                 static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADD)
                , viua::arch::Register_access::make_local(1)
                , viua::arch::Register_access::make_local(1)
                , viua::arch::Register_access::make_local(2)
            }.encode();
        } else {
            *instructions++ = viua::arch::ops::R{
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDIU)
                , viua::arch::Register_access::make_local(1)
                , viua::arch::Register_access::make_void()
                , base
            }.encode();
        }

        return instructions;
    }
    auto op_li(uint64_t* instructions, int64_t const value) -> uint64_t*
    {
        auto const parts = to_loading_parts_unsigned(value);

        /*
         * Only use the lui instruction of there's a reason to ie, if some of
         * the highest 36 bits are set. Otherwise, the lui is just overhead.
         */
        if (parts.first) {
            *instructions++ = viua::arch::ops::E{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::LUI))
                , viua::arch::Register_access::make_local(1)
                , parts.first
            }.encode();
        }

        auto const base = parts.second.first.first;
        auto const multiplier = parts.second.first.second;

        if (multiplier != 0) {
            *instructions++ = viua::arch::ops::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDI))
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_void()
                , base
            }.encode();
            *instructions++ = viua::arch::ops::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDI))
                , viua::arch::Register_access::make_local(3)
                , viua::arch::Register_access::make_void()
                , multiplier
            }.encode();
            *instructions++ = viua::arch::ops::T{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::MUL))
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(3)
            }.encode();

            auto const remainder = parts.second.second;
            *instructions++ = viua::arch::ops::R{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDI))
                , viua::arch::Register_access::make_local(3)
                , viua::arch::Register_access::make_void()
                , remainder
            }.encode();
            *instructions++ = viua::arch::ops::T{
                (viua::arch::ops::GREEDY
                 | static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADD))
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(2)
                , viua::arch::Register_access::make_local(3)
            }.encode();

            *instructions++ = viua::arch::ops::T{
                 static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADD)
                , viua::arch::Register_access::make_local(1)
                , viua::arch::Register_access::make_local(1)
                , viua::arch::Register_access::make_local(2)
            }.encode();
        } else {
            *instructions++ = viua::arch::ops::R{
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::ADDI)
                , viua::arch::Register_access::make_local(1)
                , viua::arch::Register_access::make_void()
                , base
            }.encode();
        }

        return instructions;
    }

    auto save_string(std::vector<uint8_t>& strings, std::string_view const data)
        -> size_t
    {
        auto const data_size = htole64(static_cast<uint64_t>(data.size()));
        strings.resize(strings.size() + sizeof(data_size));
        memcpy((strings.data() + strings.size() - sizeof(data_size)), &data_size, sizeof(data_size));

        auto const saved_location = strings.size();
        std::copy(data.begin(), data.end(), std::back_inserter(strings));

        return saved_location;
    }
}

namespace ast {
struct Node {
    std::vector<viua::libs::lexer::Lexeme> lexemes;
};

auto remove_noise(std::vector<viua::libs::lexer::Lexeme> raw)
    -> std::vector<viua::libs::lexer::Lexeme>
{
    using viua::libs::lexer::TOKEN;
    auto tmp = std::vector<viua::libs::lexer::Lexeme>{};
    for (auto& each : raw) {
        if (each.token == TOKEN::WHITESPACE or each.token == TOKEN::COMMENT) {
            continue;
        }

        tmp.push_back(std::move(each));
    }

    while ((not tmp.empty()) and tmp.front().token == TOKEN::TERMINATOR) {
        tmp.erase(tmp.begin());
    }

    auto cooked = std::vector<viua::libs::lexer::Lexeme>{};
    for (auto& each : tmp) {
        if (each.token != TOKEN::TERMINATOR or cooked.empty()) {
            cooked.push_back(std::move(each));
            continue;
        }

        if (cooked.back().token == TOKEN::TERMINATOR) {
            continue;
        }

        cooked.push_back(std::move(each));
    }

    return cooked;
}
}
namespace {
auto consume_token_of(
      viua::libs::lexer::TOKEN const tt
    , viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes
) -> viua::libs::lexer::Lexeme
{
    if (lexemes.front().token != tt) {
        throw lexemes.front();
    }
    auto lx  = std::move(lexemes.front());
    lexemes.remove_prefix(1);
    return lx;
}

auto parse_function_definition(viua::support::vector_view<viua::libs::lexer::Lexeme>& lexemes)
    -> std::unique_ptr<ast::Node>
{
    using viua::libs::lexer::TOKEN;

    auto const leader = consume_token_of(TOKEN::DEF_FUNCTION, lexemes);

    auto fn_name = consume_token_of(TOKEN::LITERAL_ATOM, lexemes);
    consume_token_of(TOKEN::TERMINATOR, lexemes);

    std::cerr
        << viua::libs::lexer::to_string(leader.token)
        << ' ' << fn_name.text << "\n";

    auto instructions = std::vector<std::unique_ptr<ast::Node>>{};
    while ((not lexemes.empty()) and lexemes.front() != TOKEN::END) {
        auto opcode = consume_token_of(TOKEN::OPCODE, lexemes);
        std::cerr << "  " << opcode.text << "\n";

        /*
         * Special case for instructions with no operands. It is here to make
         * the loop that extracts the operands simpler.
         */
        if (lexemes.front() == TOKEN::TERMINATOR) {
            consume_token_of(TOKEN::TERMINATOR, lexemes);
            continue;
        }

        while ((not lexemes.empty()) and lexemes.front() != TOKEN::END) {
            if (lexemes.front().token == TOKEN::END) {
                break;
            }

            /*
             * Consume the operand: void, register access, a literal value. This
             * will supply some value for the instruction to work on. This chain
             * of if-else should handle valid operands - and ONLY operands, not
             * their separators.
             */
            if (lexemes.front() == TOKEN::RA_VOID) {
                std::cerr
                    << "    "
                    << viua::libs::lexer::to_string(consume_token_of(TOKEN::RA_VOID, lexemes).token)
                    << "\n";
            } else if (lexemes.front() == TOKEN::RA_DIRECT) {
                auto const access = consume_token_of(TOKEN::RA_DIRECT, lexemes);
                auto const index = consume_token_of(TOKEN::LITERAL_INTEGER, lexemes);
                std::cerr
                    << "    "
                    << viua::libs::lexer::to_string(access.token)
                    << ' ' << access.text << index.text
                    << "\n";
            } else if (lexemes.front() == TOKEN::LITERAL_INTEGER) {
                auto const value = consume_token_of(TOKEN::LITERAL_INTEGER, lexemes);
                std::cerr
                    << "    "
                    << viua::libs::lexer::to_string(value.token)
                    << ' ' << value.text
                    << "\n";
            } else if (lexemes.front() == TOKEN::LITERAL_STRING) {
                auto const value = consume_token_of(TOKEN::LITERAL_STRING, lexemes);
                std::cerr
                    << "    "
                    << viua::libs::lexer::to_string(value.token)
                    << ' ' << value.text
                    << "\n";
            } else if (lexemes.front() == TOKEN::LITERAL_ATOM) {
                auto const value = consume_token_of(TOKEN::LITERAL_ATOM, lexemes);
                std::cerr
                    << "    "
                    << viua::libs::lexer::to_string(value.token)
                    << ' ' << value.text
                    << "\n";
            } else {
                throw lexemes.front();
            }

            /*
             * Consume either a comma (meaning that there will be some more
             * operands), or a terminator (meaning that there will be no more
             * operands).
             */
            if (lexemes.front() == TOKEN::COMMA) {
                consume_token_of(TOKEN::COMMA, lexemes);
                continue;
            }
            if (lexemes.front() == TOKEN::TERMINATOR) {
                consume_token_of(TOKEN::TERMINATOR, lexemes);
                break;
            }
            throw lexemes.front();
        }
    }

    consume_token_of(TOKEN::END, lexemes);
    consume_token_of(TOKEN::TERMINATOR, lexemes);

    return std::unique_ptr<ast::Node>();
}

auto parse(viua::support::vector_view<viua::libs::lexer::Lexeme> lexemes)
    -> std::vector<std::unique_ptr<ast::Node>>
{
    auto nodes = std::vector<std::unique_ptr<ast::Node>>{};

    std::cerr << "parse(): " << lexemes.size() << " lexeme(s)\n";

    while (not lexemes.empty()) {
        auto const& each = lexemes.front();

        using viua::libs::lexer::TOKEN;
        if (each.token == TOKEN::DEF_FUNCTION) {
            auto node = parse_function_definition(lexemes);
            nodes.push_back(std::move(node));
        } else {
            throw each;
        }
    }

    return nodes;
}
}

namespace {
auto view_line_of(std::string_view sv, viua::libs::lexer::Location loc)
    -> std::string_view
{
    // std::cerr
    //     << "want line of " << loc.line
    //     << " (from offset +" << loc.offset
    //     << ")\n";
    {
        auto line_end = size_t{0};
        line_end = sv.find('\n', loc.offset);
        // std::cerr << "  ends at " << line_end << "\n";

        if (line_end != std::string::npos) {
            sv.remove_suffix(sv.size() - line_end);
        }
    }
    {
        auto line_begin = size_t{0};
        line_begin = sv.rfind('\n', (loc.offset ? (loc.offset - 1) : 0));
        if (line_begin == std::string::npos) {
            line_begin = 0;
        }
        // std::cerr << "  begins at " << line_begin << "\n";

        sv.remove_prefix(line_begin + 1);
    }

    return sv;
}
}

auto main(int argc, char* argv[]) -> int
{
    if constexpr (false) {
        {
            auto const tm = viua::arch::ops::T{
                  0xdead
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x01}
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x02}
            };
            std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
            auto const td = viua::arch::ops::T::decode(tm.encode());
            std::cout
                << (tm.opcode == td.opcode)
                << (tm.out == td.out)
                << (tm.lhs == td.lhs)
                << (tm.rhs == td.rhs)
                << "\n";
        }
        {
            auto const tm = viua::arch::ops::D{
                  0xdead
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x01}
            };
            std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
            auto const td = viua::arch::ops::D::decode(tm.encode());
            std::cout
                << (tm.opcode == td.opcode)
                << (tm.out == td.out)
                << (tm.in == td.in)
                << "\n";
        }
        {
            auto const tm = viua::arch::ops::S{
                  0xdead
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
            };
            std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
            auto const td = viua::arch::ops::S::decode(tm.encode());
            std::cout
                << (tm.opcode == td.opcode)
                << (tm.out == td.out)
                << "\n";
        }
        {
            constexpr auto original_value = 3.14f;

            auto imm_in = uint32_t{};
            memcpy(&imm_in, &original_value, sizeof(imm_in));

            auto const tm = viua::arch::ops::F{
                  0xdead
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
                , imm_in
            };
            std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
            auto const td = viua::arch::ops::F::decode(tm.encode());

            auto imm_out = float{};
            memcpy(&imm_out, &td.immediate, sizeof(imm_out));

            std::cout
                << (tm.opcode == td.opcode)
                << (tm.out == td.out)
                << (tm.immediate == td.immediate)
                << (imm_out == original_value)
                << "\n";
        }
        {
            auto const tm = viua::arch::ops::E{
                  0xdead
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0xff}
                , 0xabcdef012
            };
            std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
            auto const td = viua::arch::ops::E::decode(tm.encode());
            std::cout
                << (tm.opcode == td.opcode)
                << (tm.out == td.out)
                << (tm.immediate == td.immediate)
                << "\n";
        }
        {
            auto const tm = viua::arch::ops::R{
                  0xdead
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x55}
                , viua::arch::Register_access{viua::arch::REGISTER_SET::LOCAL, true, 0x22}
                , 0xabcdef
            };
            std::cout << std::hex << std::setw(16) << std::setfill('0') << tm.encode() << "\n";
            auto const td = viua::arch::ops::R::decode(tm.encode());
            std::cout
                << (tm.opcode == td.opcode)
                << (tm.out == td.out)
                << (tm.in == td.in)
                << (tm.immediate == td.immediate)
                << "\n";
        }
    }

    if constexpr (false) {
        auto const test_these = std::vector<uint64_t>{
              0x0000000000000000
            , 0x0000000000000001
            , 0x0000000000bedead /* low 24 */
            , 0x00000000deadbeef /* low 32 */
            , 0xdeadbeefd0adbeef /* high 36 and low 24 (special case) */
            , 0xdeadbeefd1adbeef /* all bits */
            , 0xdeadbeefd2adbeef /* all bits */
            , 0xdeadbeefd3adbeef /* all bits */
            , 0xdeadbeefd4adbeef /* all bits */
            , 0xdeadbeefd5adbeef /* all bits */
            , 0xdeadbeefd6adbeef /* all bits */
            , 0xdeadbeefd7adbeef /* all bits */
            , 0xdeadbeefd8adbeef /* all bits */
            , 0xdeadbeefd9adbeef /* all bits */
            , 0xdeadbeefdaadbeef /* all bits */
            , 0xdeadbeefdbadbeef /* all bits */
            , 0xdeadbeefdcadbeef /* all bits */
            , 0xdeadbeefddadbeef /* all bits */
            , 0xdeadbeefdeadbeef /* all bits */
            , 0xdeadbeeffdadbeef /* all bits */
            , 0xffffffffffffffff
        };

        for (auto const wanted : test_these) {
            std::cout << "\n";

            auto const parts = to_loading_parts_unsigned(wanted);

            auto high = (parts.first << 28);
            auto const low = (parts.second.first.second != 0)
                ?  ((parts.second.first.first * parts.second.first.second)
                 + parts.second.second)
                : parts.second.first.first;
            auto const got = (high | low);

            std::cout << std::hex << std::setw(16) << std::setfill('0') << wanted << "\n";
            std::cout << std::hex << std::setw(16) << std::setfill('0') << got << "\n";
            if (wanted != got) {
                std::cerr << "BAD BAD BAD!\n";
                break;
            }
        }
    }

    if constexpr (false) {
        std::cout << viua::arch::ops::to_string(0x0000) << "\n";
        std::cout << viua::arch::ops::to_string(0x0001) << "\n";
        std::cout << viua::arch::ops::to_string(0x1001) << "\n";
        std::cout << viua::arch::ops::to_string(0x9001) << "\n";
        std::cout << viua::arch::ops::to_string(0x1002) << "\n";
        std::cout << viua::arch::ops::to_string(0x1003) << "\n";
        std::cout << viua::arch::ops::to_string(0x1004) << "\n";
        std::cout << viua::arch::ops::to_string(0x5001) << "\n";
    }

    auto args = std::vector<std::string_view>{};
    std::copy(argv + 1, argv + argc, std::back_inserter(args));

    /*
     * If invoked without any arguments, emit a sample executable binary. This
     * makes testing easy as we always can have a sample, working, known-good
     * binary produced.
     */
    if (args.empty()) {
        auto strings = std::vector<uint8_t>{};

        auto const hello_world_at = save_string(strings, "Hello, World!\n");

        std::array<viua::arch::instruction_type, 32> text {};
        auto ip = text.data();

        {
            ip = op_li(ip, 0xdeadbeefdeadbeef);
            *ip++ = viua::arch::ops::S{
                (viua::arch::ops::GREEDY |
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::DELETE))
                , viua::arch::Register_access::make_local(2)
            }.encode();
            *ip++ = viua::arch::ops::S{
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::DELETE)
                , viua::arch::Register_access::make_local(3)
            }.encode();
            *ip++ = static_cast<uint64_t>(viua::arch::ops::OPCODE::EBREAK);

            ip = op_li(ip, 42l);
            *ip++ = static_cast<uint64_t>(viua::arch::ops::OPCODE::EBREAK);

            ip = op_li(ip, -1l);
            *ip++ = viua::arch::ops::S{
                (viua::arch::ops::GREEDY |
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::DELETE))
                , viua::arch::Register_access::make_local(2)
            }.encode();
            *ip++ = viua::arch::ops::S{
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::DELETE)
                , viua::arch::Register_access::make_local(3)
            }.encode();
            *ip++ = static_cast<uint64_t>(viua::arch::ops::OPCODE::EBREAK);

            ip = op_li(ip, hello_world_at);
            *ip++ = viua::arch::ops::S{
                  static_cast<viua::arch::opcode_type>(viua::arch::ops::OPCODE::STRING)
                , viua::arch::Register_access::make_local(1)
            }.encode();
            *ip++ = static_cast<uint64_t>(viua::arch::ops::OPCODE::EBREAK);

            *ip++ = static_cast<uint64_t>(viua::arch::ops::OPCODE::HALT);
        }

        auto const a_out = open(
              "./a.out"
            , O_CREAT|O_TRUNC|O_WRONLY
            , S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH
        );
        if (a_out == -1) {
            close(a_out);
            exit(1);
        }

        constexpr auto VIUA_MAGIC[[maybe_unused]] = "\x7fVIUA\x00\x00\x00";
        auto const VIUAVM_INTERP = std::string{"viua-vm"};

        {
            auto const ops_count = (ip - text.begin());
            auto const text_size = (ops_count * sizeof(decltype(text)::value_type));

            auto const text_offset = (
                  sizeof(Elf64_Ehdr)
                + (4 * sizeof(Elf64_Phdr))
                + (VIUAVM_INTERP.size() + 1));
            auto const strings_offset = (text_offset + text_size);

            // see elf(5)
            Elf64_Ehdr elf_header {};
            elf_header.e_ident[EI_MAG0] = '\x7f';
            elf_header.e_ident[EI_MAG1] = 'E';
            elf_header.e_ident[EI_MAG2] = 'L';
            elf_header.e_ident[EI_MAG3] = 'F';
            elf_header.e_ident[EI_CLASS] = ELFCLASS64;
            elf_header.e_ident[EI_DATA] = ELFDATA2LSB;
            elf_header.e_ident[EI_VERSION] = EV_CURRENT;
            elf_header.e_ident[EI_OSABI] = ELFOSABI_STANDALONE;
            elf_header.e_ident[EI_ABIVERSION] = 0;
            elf_header.e_type = ET_EXEC;
            elf_header.e_machine = ET_NONE;
            elf_header.e_version = elf_header.e_ident[EI_VERSION];
            elf_header.e_entry = text_offset;
            elf_header.e_phoff = sizeof(elf_header);
            elf_header.e_phentsize = sizeof(Elf64_Phdr);
            elf_header.e_phnum = 4;
            elf_header.e_shoff = 0; // FIXME section header table
            elf_header.e_flags = 0; // processor-specific flags, should be 0
            elf_header.e_ehsize = sizeof(elf_header);
            write(a_out, &elf_header, sizeof(elf_header));

            Elf64_Phdr magic_for_binfmt_misc {};
            magic_for_binfmt_misc.p_type = PT_NULL;
            magic_for_binfmt_misc.p_offset = 0;
            memcpy(&magic_for_binfmt_misc.p_offset, VIUA_MAGIC, 8);
            write(a_out, &magic_for_binfmt_misc, sizeof(magic_for_binfmt_misc));

            Elf64_Phdr interpreter {};
            interpreter.p_type = PT_INTERP;
            interpreter.p_offset = (sizeof(elf_header) + 4 * sizeof(Elf64_Phdr));
            interpreter.p_filesz = VIUAVM_INTERP.size() + 1;
            interpreter.p_flags = PF_R;
            write(a_out, &interpreter, sizeof(interpreter));

            Elf64_Phdr text_segment {};
            text_segment.p_type = PT_LOAD;
            text_segment.p_offset = text_offset;
            text_segment.p_filesz = text_size;
            text_segment.p_memsz = text_size;
            text_segment.p_flags = PF_R|PF_X;
            text_segment.p_align = sizeof(viua::arch::instruction_type);
            write(a_out, &text_segment, sizeof(text_segment));

            Elf64_Phdr strings_segment {};
            strings_segment.p_type = PT_LOAD;
            strings_segment.p_offset = strings_offset;
            strings_segment.p_filesz = strings.size();
            strings_segment.p_memsz = strings.size();
            strings_segment.p_flags = PF_R;
            strings_segment.p_align = sizeof(viua::arch::instruction_type);
            write(a_out, &strings_segment, sizeof(strings_segment));

            write(a_out, VIUAVM_INTERP.c_str(), VIUAVM_INTERP.size() + 1);

            write(a_out, text.data(), text_size);

            write(a_out, strings.data(), strings.size());
        }

        close(a_out);

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
    auto const source_path = args.back();
    auto source_text = std::string{};
    {
        auto const source_fd = open(source_path.data(), O_RDONLY);

        struct stat source_stat {};
        fstat(source_fd, &source_stat);

        std::cerr << source_stat.st_size
            << " byte(s) of source code to process from "
            << source_path << "\n";

        source_text.resize(source_stat.st_size);
        read(source_fd, source_text.data(), source_text.size());
        close(source_fd);
    }

    auto lexemes = std::vector<viua::libs::lexer::Lexeme>{};
    try {
        lexemes = viua::libs::lexer::lex(source_text);
    } catch (viua::libs::lexer::Location const& e) {
        using viua::support::tty::COLOR_FG_WHITE;
        using viua::support::tty::COLOR_FG_RED;
        using viua::support::tty::ATTR_RESET;
        using viua::support::tty::send_escape_seq;

        constexpr auto esc = send_escape_seq;

        std::cerr
            << esc(2, COLOR_FG_WHITE) << source_path << esc(2, ATTR_RESET)
            << ':'<< esc(2, COLOR_FG_WHITE) << (e.line + 1) << esc(2, ATTR_RESET)
            << ':'<< esc(2, COLOR_FG_WHITE) << (e.character + 1) << esc(2, ATTR_RESET)
            << ": " << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET) << ": "
            << "no token match\n";
        return 1;
    }

    std::cerr << lexemes.size() << " raw lexeme(s)\n";
    for (auto const& each : lexemes) {
        std::cerr << "  "
            << viua::libs::lexer::to_string(each.token)
            << ' ' << each.location.line
            << ':' << each.location.character
            << '-' << (each.location.character + each.text.size() - 1)
            << " +" << each.location.offset;

        using viua::libs::lexer::TOKEN;
        auto const printable =
               (each.token == TOKEN::LITERAL_STRING)
            or (each.token == TOKEN::LITERAL_INTEGER)
            or (each.token == TOKEN::LITERAL_FLOAT)
            or (each.token == TOKEN::LITERAL_ATOM)
            or (each.token == TOKEN::OPCODE);
        if (printable) {
            std::cerr << " " << each.text;
        }

        std::cerr << "\n";
    }

    lexemes = ast::remove_noise(std::move(lexemes));
    std::cerr << lexemes.size() << " cooked lexeme(s)\n";
    if constexpr (false) {
        for (auto const& each : lexemes) {
            std::cerr << "  "
                << viua::libs::lexer::to_string(each.token)
                << ' ' << each.location.line
                << ':' << each.location.character
                << '-' << (each.location.character + each.text.size() - 1)
                << " +" << each.location.offset;

            using viua::libs::lexer::TOKEN;
            auto const printable =
                   (each.token == TOKEN::LITERAL_STRING)
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

    auto nodes = std::vector<std::unique_ptr<ast::Node>>{};
    try {
        nodes = parse(lexemes);
        std::cerr << nodes.size() << " top-level AST node(s)\n";
    } catch (viua::libs::lexer::Lexeme const& e) {
        using viua::support::tty::COLOR_FG_WHITE;
        using viua::support::tty::COLOR_FG_ORANGE_RED_1;
        using viua::support::tty::COLOR_FG_RED;
        using viua::support::tty::ATTR_RESET;
        using viua::support::tty::send_escape_seq;
        constexpr auto esc = send_escape_seq;

        auto const SEPARATOR = std::string{" |  "};
        constexpr auto LINE_NO_WIDTH = size_t{5};

        auto source_line = std::ostringstream{};
        auto highlight_line = std::ostringstream{};

        std::cerr
            << std::string(LINE_NO_WIDTH, ' ')
            << SEPARATOR << "\n";

        {
            auto const location = e.location;

            auto line = view_line_of(source_text, location);

            source_line
                << esc(2, COLOR_FG_RED)
                << std::setw(LINE_NO_WIDTH)
                << (location.line + 1)
                << esc(2, ATTR_RESET)
                << SEPARATOR;
            highlight_line
                << std::string(LINE_NO_WIDTH, ' ')
                << SEPARATOR;

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
                source_line << esc(2, COLOR_FG_RED) << e.text << esc(2, ATTR_RESET);
                line.remove_prefix(e.text.size());
            }
            highlight_line << esc(2, COLOR_FG_RED) << '^';
            highlight_line
                << esc(2, COLOR_FG_ORANGE_RED_1)
                << std::string((e.text.size() - 1), '~');

            source_line << line;
        }

        std::cerr << source_line.str() << "\n";
        std::cerr << highlight_line.str() << "\n";

        std::cerr
            << esc(2, COLOR_FG_WHITE) << source_path << esc(2, ATTR_RESET)
            << ':'<< esc(2, COLOR_FG_WHITE) << (e.location.line + 1) << esc(2, ATTR_RESET)
            << ':'<< esc(2, COLOR_FG_WHITE) << (e.location.character + 1) << esc(2, ATTR_RESET)
            << ": " << esc(2, COLOR_FG_RED) << "error" << esc(2, ATTR_RESET) << ": "
            << "unexpected token: " << viua::libs::lexer::to_string(e.token) << "\n";

        return 1;
    }

    return 0;
}
