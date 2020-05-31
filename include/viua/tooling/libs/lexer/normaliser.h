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

#ifndef VIUA_TOOLING_LIBS_LEXER_NORMALISER_H
#define VIUA_TOOLING_LIBS_LEXER_NORMALISER_H

#include <vector>

#include <viua/tooling/libs/lexer/tokenise.h>

namespace viua { namespace tooling { namespace libs { namespace lexer {
namespace normaliser {
auto normalise(std::vector<Token>) -> std::vector<Token>;
}}}}}  // namespace viua::tooling::libs::lexer::normaliser

#endif
