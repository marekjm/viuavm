/*
 *  Copyright (C) 2018 Marek Marecki
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

#include <viua/tooling/errors/compile_time.h>
#include <viua/tooling/errors/compile_time/errors.h>

namespace viua {
namespace tooling {
namespace errors {
namespace compile_time {
auto Error_wrapper::append(Error e) -> Error_wrapper& {
    fallout.emplace_back(std::move(e));
    return *this;
}

auto Error_wrapper::errors() const -> std::vector<Error> const& {
    return fallout;
}

auto Error_wrapper::errors() -> std::vector<Error>& {
    return fallout;
}
}}}}
