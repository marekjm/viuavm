#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "bytecode/bytetypedef.h"
#include "loader.h"
using namespace std;


map<string, uint16_t> Loader::loadmap(char* bytedump, const uint16_t& bytedump_size) {
    map<string, uint16_t> mapping;

    char *lib_function_ids_map = bytedump;

    int i = 0;
    string lib_fn_name;
    uint16_t lib_fn_address;
    while (i < bytedump_size) {
        lib_fn_name = string(lib_function_ids_map);
        i += lib_fn_name.size() + 1;  // one for null character
        lib_fn_address = *((uint16_t*)(bytedump+i));
        i += sizeof(uint16_t);
        lib_function_ids_map = bytedump+i;
        mapping[lib_fn_name] = lib_fn_address;
    }

    return mapping;
}

Loader& Loader::load() {
    ifstream in(path, ios::in | ios::binary);
    if (!in) {
        throw ("fatal: failed to link " + path);
    }

    unsigned lib_total_jumps;
    in.read((char*)&lib_total_jumps, sizeof(unsigned));

    unsigned lib_jmp;
    for (unsigned i = 0; i < lib_total_jumps; ++i) {
        in.read((char*)&lib_jmp, sizeof(unsigned));
        jumps.push_back(lib_jmp);
    }

    uint16_t lib_function_ids_section_size = 0;
    in.read((char*)&lib_function_ids_section_size, sizeof(uint16_t));

    char *lib_buffer_function_ids = new char[lib_function_ids_section_size];
    in.read(lib_buffer_function_ids, lib_function_ids_section_size);
    char *lib_function_ids_map = lib_buffer_function_ids;

    int i = 0;
    string lib_fn_name;
    uint16_t lib_fn_address;
    while (i < lib_function_ids_section_size) {
        lib_fn_name = string(lib_function_ids_map);
        i += lib_fn_name.size() + 1;  // one for null character
        lib_fn_address = *((uint16_t*)(lib_buffer_function_ids+i));
        i += sizeof(uint16_t);
        lib_function_ids_map = lib_buffer_function_ids+i;
        functions.push_back(lib_fn_name);
        function_addresses[lib_fn_name] = lib_fn_address;
    }
    delete[] lib_buffer_function_ids;

    in.read((char*)&size, 16);

    bytecode = new byte[size];
    in.read((char*)bytecode, size);

    return (*this);
}

Loader& Loader::executable() {
    ifstream in(path, ios::in | ios::binary);
    if (!in) {
        throw ("fatal: failed to link " + path);
    }

    uint16_t lib_function_ids_section_size = 0;
    in.read((char*)&lib_function_ids_section_size, sizeof(uint16_t));

    char *lib_buffer_function_ids = new char[lib_function_ids_section_size];
    in.read(lib_buffer_function_ids, lib_function_ids_section_size);

    for (pair<string, uint16_t> p : loadmap(lib_buffer_function_ids, lib_function_ids_section_size)) {
        functions.push_back(p.first);
        function_addresses[p.first] = p.second;
    }
    delete[] lib_buffer_function_ids;

    in.read((char*)&size, 16);

    bytecode = new byte[size];
    in.read((char*)bytecode, size);

    return (*this);
}

uint16_t Loader::getBytecodeSize() {
    return size;
}
byte* Loader::getBytecode() {
    byte* copy = new byte[size];
    for (unsigned i = 0; i < size; ++i) {
        copy[i] = bytecode[i];
    }
    return copy;
}

vector<unsigned> Loader::getJumps() {
    return jumps;
}
map<string, uint16_t> Loader::getFunctionAddresses() {
    return function_addresses;
}
vector<string> Loader::getFunctions() {
    return functions;
}
