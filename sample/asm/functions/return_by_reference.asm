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

.closure: by_reference/0
    move %0 local %1 local
    return
.end

.function: main/1
    ; store a number in register
    integer %1 local 69

    ; create a closure and capture a value by reference
    closure %2 local by_reference/0
    capture %2 local %1 %1 local

    frame %0
    ; store return value in another register (it is a reference!) local
    call %3 local %2 local

    ; assign different value to it
    integer %3 local 42

    ; check if return-by-reference is working (should print 42) local
    print %1 local

    izero %0 local
    return
.end
