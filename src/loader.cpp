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

#include <endian.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <viua/bytecode/bytetypedef.h>
#include <viua/loader.h>
#include <viua/machine.h>
#include <viua/util/memory.h>

using viua::util::memory::aligned_read;


IdToAddressMapping Loader::loadmap(char* bytedump,
                                   uint64_t const& bytedump_size)
{
    auto order = std::vector<std::string>{};
    std::map<std::string, uint64_t> mapping;

    char* lib_function_ids_map = bytedump;

    long unsigned i  = 0;
    auto lib_fn_name = std::string{};
    uint64_t lib_fn_address;
    while (i < bytedump_size) {
        lib_fn_name = std::string(lib_function_ids_map);
        i += lib_fn_name.size() + 1;  // one for null character
        aligned_read(lib_fn_address) = (bytedump + i);
        i += sizeof(decltype(lib_fn_address));
        lib_function_ids_map = bytedump + i;
        mapping[lib_fn_name] = lib_fn_address;
        order.emplace_back(lib_fn_name);
    }

    return IdToAddressMapping(order, mapping);
}
void Loader::calculate_function_sizes()
{
    for (unsigned i = 0; i < functions.size(); ++i) {
        std::string name = functions[i];
        uint64_t el_size = 0;

        if (i < (functions.size() - 1)) {
            uint64_t a = function_addresses[name];
            uint64_t b = function_addresses[functions[i + 1]];
            el_size    = (b - a);
        } else {
            uint64_t a = function_addresses[name];
            uint64_t b = size;
            el_size    = (b - a);
        }

        function_sizes[name] = el_size;
    }
}

void Loader::load_magic_number(std::ifstream& in)
{
    std::array<char, 5> magic_number {};
    in.read(magic_number.data(), magic_number.size());
    if (magic_number.back() != '\0') {
        throw "invalid magic number";
    }
    if (std::string{magic_number.data()} != std::string{VIUA_MAGIC_NUMBER}) {
        throw(std::string{"invalid magic number: "}
              + std::string{magic_number.data()});
    }
}

void Loader::assume_binary_type(std::ifstream& in,
                                Viua_binary_type assumed_binary_type)
{
    char bt;
    in.read(&bt, sizeof(decltype(bt)));
    if (bt != assumed_binary_type) {
        std::ostringstream error;
        error << "not a "
              << (assumed_binary_type == VIUA_LINKABLE ? "linkable"
                                                       : "executable")
              << " file: " << path;
        throw error.str();
    }
}

static std::map<std::string, std::string> load_meta_information_map(
    std::ifstream& in)
{
    uint64_t meta_information_map_size = 0;
    readinto(in, &meta_information_map_size);

    auto meta_information_map_buffer =
        std::make_unique<char[]>(meta_information_map_size);
    in.read(meta_information_map_buffer.get(),
            static_cast<std::streamsize>(meta_information_map_size));

    std::map<std::string, std::string> meta_information_map;

    char* buffer = meta_information_map_buffer.get();
    uint64_t i   = 0;
    std::string key, value;
    while (i < meta_information_map_size) {
        key = std::string(buffer + i);
        i += (key.size() + 1);
        value = std::string(buffer + i);
        i += (value.size() + 1);
        meta_information_map[key] = value;
    }

    return meta_information_map;
}
void Loader::load_meta_information(std::ifstream& in)
{
    meta_information = load_meta_information_map(in);
}

static std::vector<std::string> load_string_list(std::ifstream& in)
{
    uint64_t signatures_section_size = 0;
    readinto(in, &signatures_section_size);

    auto signatures_section_buffer =
        std::make_unique<char[]>(signatures_section_size);
    in.read(signatures_section_buffer.get(),
            static_cast<std::streamsize>(signatures_section_size));

    uint64_t i        = 0;
    char* buffer      = signatures_section_buffer.get();
    auto sig          = std::string{};
    auto strings_list = std::vector<std::string>{};
    while (i < signatures_section_size) {
        sig = std::string(buffer + i);
        i += (sig.size() + 1);
        strings_list.emplace_back(sig);
    }

    return strings_list;
}
void Loader::load_external_signatures(std::ifstream& in)
{
    external_signatures = load_string_list(in);
}
void Loader::load_external_block_signatures(std::ifstream& in)
{
    external_signatures_block = load_string_list(in);
}
auto Loader::load_dynamic_imports_section(std::ifstream& in) -> void
{
    dynamic_linked_modules = load_string_list(in);
}

void Loader::load_jump_table(std::ifstream& in)
{
    // load jump table
    uint64_t lib_total_jumps;
    readinto(in, &lib_total_jumps);

    uint64_t lib_jmp;
    for (uint64_t i = 0; i < lib_total_jumps; ++i) {
        readinto(in, &lib_jmp);
        jumps.push_back(lib_jmp);
    }
}
void Loader::load_functions_map(std::ifstream& in)
{
    uint64_t lib_function_ids_section_size = 0;
    readinto(in, &lib_function_ids_section_size);

    auto lib_buffer_function_ids =
        std::make_unique<char[]>(lib_function_ids_section_size);
    in.read(lib_buffer_function_ids.get(),
            static_cast<std::streamsize>(lib_function_ids_section_size));

    auto order = std::vector<std::string>{};
    std::map<std::string, uint64_t> mapping;

    tie(order, mapping) =
        loadmap(lib_buffer_function_ids.get(), lib_function_ids_section_size);

    for (std::string p : order) {
        functions.emplace_back(p);
        function_addresses[p] = mapping[p];
    }
}
void Loader::load_blocks_map(std::ifstream& in)
{
    uint64_t lib_block_ids_section_size = 0;
    readinto(in, &lib_block_ids_section_size);

    auto lib_buffer_block_ids =
        std::make_unique<char[]>(lib_block_ids_section_size);
    in.read(lib_buffer_block_ids.get(),
            static_cast<std::streamsize>(lib_block_ids_section_size));

    auto order = std::vector<std::string>{};
    std::map<std::string, uint64_t> mapping;

    tie(order, mapping) =
        loadmap(lib_buffer_block_ids.get(), lib_block_ids_section_size);

    for (std::string p : order) {
        blocks.emplace_back(p);
        block_addresses[p] = mapping[p];
    }
}
void Loader::load_bytecode(std::ifstream& in)
{
    in.read(reinterpret_cast<char*>(&size), sizeof(decltype(size)));
    bytecode = std::make_unique<uint8_t[]>(size);
    in.read(reinterpret_cast<char*>(bytecode.get()),
            static_cast<std::streamsize>(size));
}

Loader& Loader::load()
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        throw("failed to open file: " + path);
    }

    load_magic_number(in);
    assume_binary_type(in, VIUA_LINKABLE);

    load_meta_information(in);

    // jump table must be loaded if loading a library
    load_jump_table(in);

    load_external_signatures(in);
    load_external_block_signatures(in);
    load_dynamic_imports_section(in);
    load_blocks_map(in);
    load_functions_map(in);
    load_bytecode(in);
    calculate_function_sizes();

    return (*this);
}

Loader& Loader::executable()
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        throw("fatal: failed to open file: " + path);
    }

    load_magic_number(in);
    assume_binary_type(in, VIUA_EXECUTABLE);

    load_meta_information(in);

    load_external_signatures(in);
    load_external_block_signatures(in);
    load_dynamic_imports_section(in);
    load_blocks_map(in);
    load_functions_map(in);
    load_bytecode(in);
    calculate_function_sizes();

    return (*this);
}

uint64_t Loader::get_bytecode_size()
{
    return size;
}
auto Loader::get_bytecode() -> std::unique_ptr<uint8_t[]>
{
    auto copy = std::make_unique<uint8_t[]>(size);
    for (uint64_t i = 0; i < size; ++i) {
        copy[i] = bytecode[i];
    }
    return copy;
}

auto Loader::get_jumps() -> std::vector<uint64_t>
{
    return jumps;
}

auto Loader::get_meta_information() -> std::map<std::string, std::string>
{
    return meta_information;
}

auto Loader::get_external_signatures() -> std::vector<std::string>
{
    return external_signatures;
}

auto Loader::get_external_block_signatures() -> std::vector<std::string>
{
    return external_signatures_block;
}

auto Loader::get_function_addresses() -> std::map<std::string, uint64_t>
{
    return function_addresses;
}
auto Loader::get_function_sizes() -> std::map<std::string, uint64_t>
{
    return function_sizes;
}
auto Loader::get_functions() -> std::vector<std::string>
{
    return functions;
}

auto Loader::get_block_addresses() -> std::map<std::string, uint64_t>
{
    return block_addresses;
}
auto Loader::get_blocks() -> std::vector<std::string>
{
    return blocks;
}

auto Loader::dynamic_imports() const -> std::vector<std::string>
{
    return dynamic_linked_modules;
}
