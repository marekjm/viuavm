/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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

#include <algorithm>
#include <string>
#include <tuple>

#include <viua/bytecode/codec/main.h>


// Helper functions for checking if a container contains an item.
template<typename T> auto in(std::vector<T> v, T item) -> bool
{
    return (std::find(v.begin(), v.end(), item) != v.end());
}
template<typename K, typename V> auto in(std::map<K, V> m, K key) -> bool
{
    return (m.count(key) == 1);
}

namespace disassembler {
using Decoder_type = viua::bytecode::codec::main::Decoder;

auto instruction(Decoder_type const&, uint8_t const*)
    -> std::tuple<std::string, viua::internals::types::bytecode_size>;
}  // namespace disassembler


#endif
