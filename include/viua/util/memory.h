/*
 *  Copyright (C) 2016 Marek Marecki
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

#ifndef VIUA_UTIL_MEMORY_H
#define VIUA_UTIL_MEMORY_H

#pragma once


namespace viua {
    namespace util {
        namespace memory {
            template<class T> class maybe_unique_ptr {
                bool owns_pointer;
                T* pointer;

                auto delete_if_owned() -> void {
                    if (owns_pointer and (pointer != nullptr)) {
                        delete pointer;
                        pointer = nullptr;
                    }
                }

                public:
                auto release() -> T* {
                    auto tmp = pointer;
                    pointer = nullptr;
                    owns_pointer = false;
                    return tmp;
                }
                auto reset(T* ptr, bool own = true) -> void {
                    delete_if_owned();
                    owns_pointer = own;
                    pointer = ptr;
                }

                auto get() -> T* {
                    return pointer;
                }

                auto owns() const -> bool {
                    return owns_pointer;
                }

                auto operator->() -> T* {
                    return pointer;
                }

                maybe_unique_ptr(T* ptr = nullptr, bool own = true): owns_pointer(own), pointer(ptr) {
                }
                ~maybe_unique_ptr() {
                    delete_if_owned();
                }
            };
        }
    }
}


#endif
