;
;   Copyright (C) 2015, 2016 Marek Marecki
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

.function: by_reference/0
    move 0 1
    return
.end

.function: main/1
    ; store a number in register
    istore 1 69

    ; create a closure and enclose a value by reference
    closure 2 by_reference/0
    enclose 2 1 1

    frame 0
    ; store return value in another register (it is a reference!)
    fcall 3 2

    ; assign different value to it
    istore 3 42

    ; check if return-by-reference is working (should print 42)
    print 1

    izero 0
    return
.end
