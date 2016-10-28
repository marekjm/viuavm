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

.function: adder/1
    ; expects register 1 to be an captured integer
    arg 2 0
    iadd 0 2 1
    return
.end

.function: make_adder/1
    ; takes an integer and returns a function of one argument adding
    ; given integer to its only parameter
    ;
    ; example:
    ;   make_adder(3)(5) == 8
    .name: 1 number
    arg number 0
    closure 2 adder/1
    capture 2 1 1
    move 0 2
    return
.end

.function: main/1
    ; create the adder function
    .name: 2 add_three
    frame ^[(param 0 (istore 1 3))]
    call add_three make_adder/1

    ; add_three(2)
    frame ^[(param 0 (istore 3 2))]
    print (fcall 4 add_three)

    ; add_three(5)
    frame ^[(param 0 (istore 3 5))]
    print (fcall 4 add_three)

    ; add_three(13)
    frame ^[(param 0 (istore 3 13))]
    print (fcall 4 add_three)

    izero 0
    return
.end
