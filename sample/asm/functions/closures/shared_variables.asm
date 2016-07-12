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

.function: closure_printer/0
    ; it has to be 2, because 2 register has been bound
    print 2
    return
.end

.function: closure_setter/1
    arg 1 0

    ; overwrite bound value with whatever we got
    istore 2 @1
    return
.end

.function: returns_closures/0
    ; create a vector to store closures
    vec 1

    ; create a value to be bound in both closures
    istore 2 42

    ; create two closures binding the same variable
    ; presto, we have two functions that are share some state
    closure 3 closure_printer/0
    enclose 3 2 2

    closure 4 closure_setter/1
    enclose 4 2 2

    ; push closures to vector...
    vpush 1 3
    vpush 1 4

    ; ...and return the vector
    ; vectors can be used to return multiple values as
    ; they can hold any Type-derived type
    move 0 1
    return
.end

.function: main/1
    frame 0
    call 1 returns_closures/0

    frame 0
    fcall 0 (vat 2 1 0)

    frame ^[(param 0 (istore 4 69))]
    fcall 0 (vat 3 1 1)

    frame 0
    fcall 0 2

    izero 0
    return
.end
