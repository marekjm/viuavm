/*
 *  Copyright (C) 2016, 2017 Marek Marecki
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

#include <cstring>
#include <memory>


#pragma once


namespace viua { namespace util { namespace memory {
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
        auto tmp     = pointer;
        pointer      = nullptr;
        owns_pointer = false;
        return tmp;
    }
    auto reset(T* ptr, bool own = true) -> void {
        delete_if_owned();
        owns_pointer = own;
        pointer      = ptr;
    }
    auto reset(std::unique_ptr<T> ptr) -> void {
        delete_if_owned();
        owns_pointer = true;
        pointer      = ptr.release();
    }

    auto operator=(std::unique_ptr<T> ptr) -> maybe_unique_ptr& {
        reset(std::move(ptr));
        return *this;
    }

    auto get() -> T* {
        return pointer;
    }

    auto owns() const -> bool {
        return owns_pointer;
    }

    auto operator-> () -> T* {
        return pointer;
    }

    maybe_unique_ptr(T* ptr = nullptr, bool own = true)
            : owns_pointer(own), pointer(ptr) {}
    ~maybe_unique_ptr() {
        delete_if_owned();
    }
};

template<class To, class From> auto load_aligned(const From* source) -> To {
    To data{};
    std::memcpy(&data, source, sizeof(To));
    return data;
}

template<class To> class aligned_write_impl {
    To* target;

  public:
    aligned_write_impl(To* t) : target(t) {}

    template<class From>
    auto operator=(const From source) -> aligned_write_impl& {
        std::memcpy(target, &source, sizeof(From));
        return *this;
    }

    template<class From>
    auto operator+=(const From source) -> aligned_write_impl& {
        From data = load_aligned<From>(target);
        data += source;
        std::memcpy(target, &data, sizeof(From));
        return *this;
    }
};

template<class To> auto aligned_write(To* target) -> aligned_write_impl<To> {
    return aligned_write_impl<To>(target);
}

template<class To> class aligned_read_impl {
    To& target;

  public:
    aligned_read_impl(To& t) : target(t) {}

    template<class From>
    auto operator=(const From* source) -> aligned_read_impl& {
        std::memcpy(&target, source, sizeof(target));
        return *this;
    }
};

template<class To> auto aligned_read(To& target) -> aligned_read_impl<To> {
    return aligned_read_impl<To>(target);
}
}}}  // namespace viua::util::memory


#endif
