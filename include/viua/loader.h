/*
 *  Copyright (C) 2015, 2016 Marek Marecki
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

#ifndef VIUA_LOADER_H
#define VIUA_LOADER_H

#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <viua/bytecode/bytetypedef.h>
#include <viua/machine.h>

typedef std::tuple<std::vector<std::string>,
                   std::map<std::string, viua::internals::types::bytecode_size>>
    IdToAddressMapping;

template<class T> void readinto(std::ifstream& in, T* object)
{
    in.read(reinterpret_cast<char*>(object), sizeof(T));
}

class Loader {
    std::string path;

    viua::internals::types::bytecode_size size;
    std::unique_ptr<uint8_t[]> bytecode;

    std::vector<viua::internals::types::bytecode_size> jumps;

    std::map<std::string, std::string> meta_information;

    std::vector<std::string> external_signatures;
    std::vector<std::string> external_signatures_block;

    std::vector<std::string> dynamic_linked_modules;

    std::map<std::string, viua::internals::types::bytecode_size>
        function_addresses;
    std::map<std::string, viua::internals::types::bytecode_size> function_sizes;
    std::vector<std::string> functions;
    std::map<std::string, viua::internals::types::bytecode_size>
        block_addresses;
    std::vector<std::string> blocks;

    IdToAddressMapping loadmap(char*,
                               viua::internals::types::bytecode_size const&);
    void calculate_function_sizes();

    void load_magic_number(std::ifstream&);
    void assume_binary_type(std::ifstream&, Viua_binary_type);

    void load_meta_information(std::ifstream&);

    void load_external_signatures(std::ifstream&);
    void load_external_block_signatures(std::ifstream&);
    auto load_dynamic_imports_section(std::ifstream&) -> void;
    void load_jump_table(std::ifstream&);
    void load_functions_map(std::ifstream&);
    void load_blocks_map(std::ifstream&);
    void load_bytecode(std::ifstream&);

  public:
    Loader& load();
    Loader& executable();

    viua::internals::types::bytecode_size get_bytecode_size();
    std::unique_ptr<uint8_t[]> get_bytecode();

    std::vector<viua::internals::types::bytecode_size> get_jumps();

    std::map<std::string, std::string> get_meta_information();

    std::vector<std::string> get_external_signatures();
    std::vector<std::string> get_external_block_signatures();

    std::map<std::string, viua::internals::types::bytecode_size>
    get_function_addresses();
    std::map<std::string, viua::internals::types::bytecode_size>
    get_function_sizes();
    std::vector<std::string> get_functions();

    std::map<std::string, viua::internals::types::bytecode_size>
    get_block_addresses();
    std::vector<std::string> get_blocks();

    auto dynamic_imports() const -> std::vector<std::string>;

    Loader(std::string pth) : path(pth), size(0), bytecode(nullptr) {}
    ~Loader() {}
};


#endif
