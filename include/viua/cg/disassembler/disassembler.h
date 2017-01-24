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

#ifndef VIUA_DISASSEMBLER_H
#define VIUA_DISASSEMBLER_H

#pragma once

#include <algorithm>
#include <string>
#include <tuple>


// Helper functions for checking if a container contains an item.
template<typename T> bool in(std::vector<T> v, T item) {
    return (std::find(v.begin(), v.end(), item) != v.end());
}
template<typename K, typename V> bool in(std::map<K, V> m, K key) {
    return (m.count(key) == 1);
}

namespace disassembler {
    std::string intop(viua::internals::types::byte*);
    std::string intop_with_rs_type(viua::internals::types::byte*);
    std::tuple<std::string, viua::internals::types::bytecode_size> instruction(viua::internals::types::byte*);
}


#endif
