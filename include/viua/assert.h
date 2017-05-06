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

#ifndef VIUA_ASSERT_H
#define VIUA_ASSERT_H

#pragma once

#include <string>
#include <viua/kernel/frame.h>
#include <viua/types/value.h>
#include <viua/exceptions.h>


namespace viua {
    namespace assertions {
        void assert_typeof(viua::types::Value* object, const std::string& expected);

        template<typename T, typename U> inline bool any_equal(const T& to_compare, const U& first) {
            return (to_compare == first);
        }
        template<typename T, typename U, typename... R> bool any_equal(const T& to_compare, const U& first, const R&... rest) {
            return ((to_compare == first) or any_equal(to_compare, rest...));
        }

        using Arity = viua::internals::types::register_index;

        template<typename... A> void assert_arity(const Frame* frame, const A&... valid_arities) {
            Arity arity = frame->arguments->size();
            if (not any_equal(arity, valid_arities...)) {
                throw new ArityException(arity, {valid_arities...});
            }
        }
    }
}



#endif
