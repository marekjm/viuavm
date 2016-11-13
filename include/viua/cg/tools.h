#ifndef VIUA_CG_TOOLS_H
#define VIUA_CG_TOOLS_H

#pragma once

#include <cstdint>
#include <viua/bytecode/maps.h>
#include <viua/cg/lex.h>


namespace viua {
    namespace cg {
        namespace tools {
            template<class T> static bool any(T item, T other) {
                return (item == other);
            }
            template<class T, class... R> static bool any(T item, T first, R... rest) {
                if (item == first) {
                    return true;
                }
                return any(item, rest...);
            }

            OPCODE mnemonic_to_opcode(const std::string&);
            uint64_t calculate_bytecode_size(const std::vector<viua::cg::lex::Token>&);
            uint64_t calculate_bytecode_size_in_range(const std::vector<viua::cg::lex::Token>& tokens, std::remove_reference<decltype(tokens)>::type::size_type limit);
            uint64_t calculate_bytecode_size_of_first_n_instructions(const std::vector<viua::cg::lex::Token>& tokens, std::remove_reference<decltype(tokens)>::type::size_type limit);
            uint64_t calculate_bytecode_size2(const std::vector<viua::cg::lex::Token>&);
        }
    }
}


#endif
