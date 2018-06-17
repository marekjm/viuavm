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

.function: adder/1
    ; expects register 1 to be an captured integer
    arg %2 local %0
    add %0 local %2 local %1 local
    return
.end

.function: make_adder/1
    ; takes an integer and returns a function of one argument adding
    ; given integer to its only parameter
    ;
    ; example:
    ;   make_adder(3)(5) == 8
    .name: 1 number
    arg %number local %0
    closure %2 local adder/1
    capture %2 local %1 %1 local
    move %0 local %2 local
    return
.end

.function: main/1
    ; create the adder function
    .name: 2 add_three
    frame ^[(param %0 (integer %1 local 3) local)]
    call %add_three local make_adder/1

    ; add_three(2)
    frame ^[(param %0 (integer %3 local 2) local)]
    print (call %4 local %add_three local) local

    ; add_three(5)
    frame ^[(param %0 (integer %3 local 5) local)]
    print (call %4 local %add_three local) local

    ; add_three(13)
    frame ^[(param %0 (integer %3 local 13))]
    print (call %4 local %add_three local) local

    izero %0 local
    return
.end
