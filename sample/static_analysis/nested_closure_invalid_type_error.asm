;
;   Copyright (C) 2017, 2018 Marek Marecki <marekjm@ozro.pw>
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

.closure: b_closure/0
    ; (5) Here we trigger a type error because
    ; in the main/0 function (where the capturing
    ; started) the value had 'text' type, and
    ; here is used as an integer.
    iinc %2 local
    print %2 local
    return
.end

.closure: a_closure/0
    ; (3) Here, we create a nested closure...
    closure %1 local b_closure/0
    ; (4) ...which does not care about the type
    ; of the captured value, and
    ; just moves it into the nested closure.
    capturemove %1 local %2 %2 local

    frame %0
    call void %1 local

    return
.end

.function: main/1
    allocate_registers %3 local

    ; (1) We create a closure here...
    closure %1 local a_closure/0
    text %2 local "Hello World!"

    ; (2) ...and capture a text value in the
    ; local register 2 of the newly created closure.
    capturemove %1 local %2 %2 local

    frame %0
    call void %1 local

    izero %0 local
    return
.end
