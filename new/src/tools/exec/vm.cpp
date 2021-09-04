#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <viua/arch/arch.h>
#include <viua/arch/ins.h>
#include <viua/arch/ops.h>
#include <viua/support/string.h>
#include <viua/vm/core.h>
#include <viua/vm/types.h>
#include <viua/vm/ins.h>


using viua::vm::Stack;
using viua::vm::Env;

auto Env::function_at(size_t const offset) const -> std::pair<std::string, size_t>
{
    auto sz = uint64_t{};
    memcpy(&sz, (functions_table.data() + offset - sizeof(sz)), sizeof(sz));
    sz = le64toh(sz);

    auto name = std::string{reinterpret_cast<char const*>(functions_table.data()) + offset, sz};

    auto addr = uint64_t{};
    memcpy(&addr, (functions_table.data() + offset + sz), sizeof(addr));
    addr = le64toh(addr);

    return { name, addr };
}

namespace {
auto execute(Stack& stack,
             Env& env,
             viua::arch::instruction_type const* const ip)
    -> viua::arch::instruction_type const*
{
    auto const raw = *ip;

    auto const opcode = static_cast<viua::arch::opcode_type>(
        raw & viua::arch::ops::OPCODE_MASK);
    auto const format = static_cast<viua::arch::ops::FORMAT>(
        opcode & viua::arch::ops::FORMAT_MASK);

    switch (format) {
    using viua::vm::ins::execute;
    using namespace viua::arch::ins;
    case viua::arch::ops::FORMAT::T:
    {
        auto instruction = viua::arch::ops::T::decode(raw);
        switch (static_cast<viua::arch::ops::OPCODE_T>(opcode)) {
        case viua::arch::ops::OPCODE_T::ADD:
            execute(ADD{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::SUB:
            execute(SUB{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::MUL:
            execute(MUL{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::DIV:
            execute(DIV{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::MOD:
            execute(MOD{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::BITSHL:
            execute(BITSHL{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::BITSHR:
            execute(BITSHR{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::BITASHR:
            execute(BITASHR{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::BITROL:
            execute(BITROL{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::BITROR:
            execute(BITROR{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::BITAND:
            execute(BITAND{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::BITOR:
            execute(BITOR{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::BITXOR:
            execute(BITXOR{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::EQ:
            execute(EQ{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::GT:
            execute(GT{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::LT:
            execute(LT{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::CMP:
            execute(CMP{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_T::AND:
            execute(stack.back().registers, viua::arch::ins::AND{instruction});
            break;
        case viua::arch::ops::OPCODE_T::OR:
            execute(stack.back().registers, viua::arch::ins::OR{instruction});
            break;
        }
        break;
    }
    case viua::arch::ops::FORMAT::S:
    {
        auto instruction = viua::arch::ops::S::decode(raw);
        switch (static_cast<viua::arch::ops::OPCODE_S>(opcode)) {
        case viua::arch::ops::OPCODE_S::DELETE:
            execute(DELETE{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_S::STRING:
            execute(STRING{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_S::FRAME:
            execute(FRAME{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_S::RETURN:
            return execute(RETURN{instruction}, stack, ip);
        case viua::arch::ops::OPCODE_S::FLOAT:
            execute(stack,
                    ip,
                    env,
                    viua::arch::ins::FLOAT{instruction});
            break;
        case viua::arch::ops::OPCODE_S::DOUBLE:
            execute(stack,
                    ip,
                    env,
                    viua::arch::ins::DOUBLE{instruction});
            break;
        }
        break;
    }
    case viua::arch::ops::FORMAT::E:
    {
        auto instruction = viua::arch::ops::E::decode(raw);
        switch (static_cast<viua::arch::ops::OPCODE_E>(opcode)) {
        case viua::arch::ops::OPCODE_E::LUI:
            execute(stack.back().registers, viua::arch::ins::LUI{instruction});
            break;
        case viua::arch::ops::OPCODE_E::LUIU:
            execute(stack.back().registers, viua::arch::ins::LUIU{instruction});
            break;
        }
        break;
    }
    case viua::arch::ops::FORMAT::R:
    {
        auto instruction = viua::arch::ops::R::decode(raw);
        switch (static_cast<viua::arch::ops::OPCODE_R>(opcode)) {
        case viua::arch::ops::OPCODE_R::ADDI:
            execute(ADDI{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_R::ADDIU:
            execute(ADDIU{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_R::SUBI:
            execute(SUBI{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_R::SUBIU:
            execute(SUBIU{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_R::MULI:
            execute(MULI{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_R::MULIU:
            execute(MULIU{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_R::DIVI:
            execute(DIVI{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_R::DIVIU:
            execute(DIVIU{instruction}, stack, ip);
            break;
        }
        break;
    }
    case viua::arch::ops::FORMAT::N:
    {
        std::cerr << "    " + viua::arch::ops::to_string(opcode) + "\n";
        switch (static_cast<viua::arch::ops::OPCODE_N>(opcode)) {
        case viua::arch::ops::OPCODE_N::NOOP:
            break;
        case viua::arch::ops::OPCODE_N::HALT:
            return nullptr;
        case viua::arch::ops::OPCODE_N::EBREAK:
            execute(EBREAK{viua::arch::ops::N::decode(raw)}, stack, ip);
            break;
        }
        break;
    }
    case viua::arch::ops::FORMAT::D:
    {
        auto instruction = viua::arch::ops::D::decode(raw);
        switch (static_cast<viua::arch::ops::OPCODE_D>(opcode)) {
        case viua::arch::ops::OPCODE_D::CALL:
            /*
             * Call is a special instruction. It transfers the IP to a
             * semi-random location, instead of just increasing it to the next
             * unit.
             *
             * This is why we return here, and not use the default behaviour for
             * most of the other instructions.
             */
            return execute(CALL{instruction}, stack, ip);
        case viua::arch::ops::OPCODE_D::BITNOT:
            execute(BITNOT{instruction}, stack, ip);
            break;
        case viua::arch::ops::OPCODE_D::NOT:
            execute(stack.back().registers, viua::arch::ins::NOT{instruction});
            break;
        }
        break;
    }
    case viua::arch::ops::FORMAT::F:
        std::cerr << "unimplemented instruction: "
                  << viua::arch::ops::to_string(opcode) << "\n";
        return nullptr;
    }

    return (ip + 1);
}

auto run_instruction(Stack& stack,
                     Env& env,
                     viua::arch::instruction_type const* ip)
    -> viua::arch::instruction_type const*
{
    auto instruction = viua::arch::instruction_type{};
    do {
        instruction = *ip;
        ip          = execute(stack, env, ip);
    } while ((ip != nullptr) and (instruction & viua::arch::ops::GREEDY));

    return ip;
}

auto run(Stack& stack,
         Env& env,
         viua::arch::instruction_type const* ip,
         std::tuple<std::string_view const,
                    viua::arch::instruction_type const*,
                    viua::arch::instruction_type const*> ip_range) -> void
{
    auto const [module, ip_begin, ip_end] = ip_range;

    constexpr auto PREEMPTION_THRESHOLD = size_t{2};

    while ((ip < ip_end) and (ip >= ip_begin)) {
        std::cerr << "cycle at " << module << "+0x" << std::hex << std::setw(8)
                  << std::setfill('0')
                  << ((ip - ip_begin) * sizeof(viua::arch::instruction_type))
                  << std::dec << "\n";

        for (auto i = size_t{0}; i < PREEMPTION_THRESHOLD and ip != ip_end;
             ++i) {
            /*
             * This is needed to detect greedy bundles and adjust preemption
             * counter appropriately. If a greedy bundle contains more
             * instructions than the preemption threshold allows the process
             * will be suspended immediately.
             */
            auto const greedy    = (*ip & viua::arch::ops::GREEDY);
            auto const bundle_ip = ip;

            ip = run_instruction(stack, env, ip);

            /*
             * Halting instruction returns nullptr because it does not know
             * where the end of bytecode lies. This is why we have to watch
             * out for null pointer here.
             */
            ip = (ip == nullptr ? ip_end : ip);

            /*
             * If the instruction was a greedy bundle instead of a single
             * one, the preemption counter has to be adjusted. It may be the
             * case that the bundle already hit the preemption threshold.
             */
            if (greedy and ip != ip_end) {
                i += (ip - bundle_ip) - 1;
            }
        }

        if (stack.frames.empty()) {
            std::cerr << "exited\n";
            break;
        }
        if (ip == ip_end) {
            std::cerr << "halted\n";
            break;
        }

        if constexpr (false) {
            /*
             * FIXME Limit the amount of instructions executed per second
             * for debugging purposes. Once everything works as it should,
             * remove this code.
             */
            using namespace std::literals;
            std::this_thread::sleep_for(160ms);
        }
    }

    if ((ip > ip_end) or (ip < ip_begin)) {
        std::cerr << "[vm] ip " << std::hex << std::setw(8) << std::setfill('0')
                  << ((ip - ip_begin) * sizeof(viua::arch::instruction_type))
                  << std::dec << " outside of valid range\n";
    }
}
}  // namespace

auto main(int argc, char* argv[]) -> int
{
    /*
     * If invoked with some operands, use the first of them as the
     * binary to load and execute. It most probably will be the sample
     * executable generated by an earlier invokation of the codec
     * testing program.
     */
    auto const executable_path = std::string{(argc > 1) ? argv[1] : "./a.out"};

    auto strings = std::vector<uint8_t>{};
    auto fn_table = std::vector<uint8_t>{};
    auto text    = std::vector<viua::arch::instruction_type>{};

    auto entry_addr = size_t{0};

    {
        auto const a_out = open(executable_path.c_str(), O_RDONLY);
        if (a_out == -1) {
            close(a_out);
            exit(1);
        }

        Elf64_Ehdr elf_header{};
        read(a_out, &elf_header, sizeof(elf_header));

        Elf64_Phdr program_header{};

        /*
         * We need to skip a few program headers which are just used to make
         * the file a proper ELF as recognised by file(1) and readelf(1).
         */
        read(a_out, &program_header, sizeof(program_header));  // skip magic
                                                               // PT_NULL
        read(a_out, &program_header, sizeof(program_header));  // skip PT_INTERP

        /*
         * Then come the actually useful program headers describing PT_LOAD
         * segments with .text section (containing the instructions we need to
         * run the program), and .rodata (containing the strings representing
         * strings and symbols).
         */
        Elf64_Phdr text_header{};
        read(a_out, &text_header, sizeof(text_header));

        Elf64_Phdr strings_header{};
        read(a_out, &strings_header, sizeof(strings_header));

        Elf64_Phdr fn_header{};
        read(a_out, &fn_header, sizeof(fn_header));

        lseek(a_out, text_header.p_offset, SEEK_SET);
        text.resize(text_header.p_filesz
                    / sizeof(viua::arch::instruction_type));
        read(a_out, text.data(), text_header.p_filesz);

        if (strings_header.p_filesz) {
            lseek(a_out, strings_header.p_offset, SEEK_SET);
            strings.resize(strings_header.p_filesz);
            read(a_out, strings.data(), strings_header.p_filesz);
        }
        if (fn_header.p_filesz) {
            lseek(a_out, fn_header.p_offset, SEEK_SET);
            fn_table.resize(fn_header.p_filesz);
            read(a_out, fn_table.data(), fn_header.p_filesz);
        }

        entry_addr = ((elf_header.e_entry - text_header.p_offset) / sizeof(viua::arch::instruction_type));

        std::cout << "[vm] loaded " << text_header.p_filesz
                  << " byte(s) of .text section from PT_LOAD segment of "
                  << executable_path << "\n";
        std::cout << "[vm] loaded "
                  << (text_header.p_filesz / sizeof(decltype(text)::value_type))
                  << " instructions\n";
        std::cout << "[vm] .text address at +0x" << std::hex << std::setw(8)
                  << std::setfill('0')
                  << text_header.p_offset
                  << std::dec << "\n";
        std::cout << "[vm] ELF entry address at +0x" << std::hex << std::setw(8)
                  << std::setfill('0')
                  << elf_header.e_entry
                  << std::dec << "\n";
        std::cout << "[vm] entry address at [.text]+0x" << std::hex << std::setw(8)
                  << std::setfill('0')
                  << (entry_addr * sizeof(viua::arch::instruction_type))
                  << std::dec << "\n";
        std::cout
            << "[vm] loaded " << strings_header.p_filesz
            << " byte(s) of .rodata (strings) section from PT_LOAD segment of "
            << executable_path << "\n";
        std::cout
            << "[vm] loaded " << fn_header.p_filesz
            << " byte(s) of .rodata (fn table) section from PT_LOAD segment of "
            << executable_path << "\n";

        close(a_out);
    }

    auto env = Env{};
    env.strings_table = std::move(strings);
    env.functions_table = std::move(fn_table);
    env.ip_base = &text[0];

    auto stack = Stack{env};
    stack.push(256, (text.data() + entry_addr), nullptr);

    auto const ip_begin = &text[0];
    auto const ip_end   = (ip_begin + text.size());
    try {
        run(stack,
            env,
            stack.back().entry_address,
            {(executable_path + "[.text]"), ip_begin, ip_end});
    } catch (viua::vm::abort_execution const& e) {
        std::cerr << "Aborted execution: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
