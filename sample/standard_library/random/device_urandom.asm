;
;   Copyright (C) 2015, 2016, 2017 Marek Marecki
;
;   This file is part of Viua VM.
;
;   Viua VM is free software: you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, either version 3 of the License, or
;   (at your option) any later version.
;
;   Viua VM is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
;

.signature: std::random::device::urandom

.function: main/1
    ; first, import the random module to make std::random functions available
    import "random"

    ; this is a nonblocking call
    ; if gathered entropy bytes are not sufficient to form an integer,
    ; pseudo-random bytes will be used
    frame %0
    print (call %1 local std::random::device::urandom) local

    izero %0 local
    return
.end
