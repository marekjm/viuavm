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
#include <viua/types/type.h>
#include <viua/exceptions.h>


namespace viua {
    namespace assertions {
        template<class T> void assert_implements(viua::types::Type* object, const std::string& s) {
            /** Use this assertion when casting to interface type (C++ abstract class).
             *
             *  Example: casting Vector to Iterator.
             */
            if (dynamic_cast<T*>(object) == nullptr) {
                throw new viua::types::Exception(object->type() + " does not implement " + s);
            }
        }

        void assert_typeof(viua::types::Type* object, const std::string& expected);

        template<class T> T* expect_type(const std::string& expected_1st, viua::types::Type* got_1st) {
            T* ptr = dynamic_cast<T*>(got_1st);
            if (not ptr) {
                throw new viua::types::Exception("invalid operand types: expected (" + expected_1st + "), got (" + got_1st->type() + ")");
            }
            return ptr;
        }

        template<class T, class U> void expect_types(const std::string& expected_1st, const std::string& expected_2nd, viua::types::Type* got_1st, viua::types::Type* got_2nd) {
            if ((not dynamic_cast<T*>(got_1st)) or (not dynamic_cast<U*>(got_2nd))) {
                throw new viua::types::Exception("invalid operand types: expected (_, " + expected_1st + ", " + expected_2nd + "), got (_, " + got_1st->type() + ", " + got_2nd->type() + ")");
            }
        }
        template<class T> void expect_types(const std::string& expected_both, viua::types::Type* got_1st, viua::types::Type* got_2nd) {
            expect_types<T, T>(expected_both, expected_both, got_1st, got_2nd);
        }

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
