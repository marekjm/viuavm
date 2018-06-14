;
;   Copyright (C) 2017 Marek Marecki <marekjm@ozro.pw>
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

.function: bottles_of_beer_text/1
    .name: %iota number_of_bottles
    arg %number_of_bottles local %0

    .name: %iota bottles_of_beer
    ; support for "1 bottle of beer" and "N bottles of beer"
    if (eq %iota local %number_of_bottles local (integer %iota local 1) local) local +1 +3
    text %bottles_of_beer local " bottle of beer"
    jump +2
    text %bottles_of_beer local " bottles of beer"

    move %0 local %bottles_of_beer local
    return
.end

.function: first_print/1
    ; this function prints the
    ;
    ;   N bottles of beer on the wall
    ;   N bottles of beer
    ;   Take one down, pass it around
    ;
    .name: %iota number_of_bottles
    arg %number_of_bottles local %0

    .name: %iota bottles_of_beer
    frame ^[(param %0 %number_of_bottles local)]
    call %bottles_of_beer local bottles_of_beer_text/1

    echo %number_of_bottles local
    echo %bottles_of_beer local
    print (text %iota local " on the wall") local
    echo %number_of_bottles local
    print %bottles_of_beer local
    print (text %iota local "Take one down, pass it around") local

    return
.end

.function: second_print/1
    ; this function prints the
    ;
    ;   No more bottles of beer on the wall /
    ;   N bottles of beer on the wall
    ;
    ; i.e. the last line of a paragraph
    .name: %iota number_of_bottles
    arg %number_of_bottles %0

    .name: %iota bottles_of_beer
    frame ^[(param %0 %number_of_bottles local)]
    call %bottles_of_beer local bottles_of_beer_text/1

    .name: %iota on_the_wall
    text %on_the_wall local " on the wall"

    ; say "No more" instead of "0 bottles"
    if %number_of_bottles local +1 +3
    echo %number_of_bottles local
    jump +3
    echo (text %iota local "No more") local

    echo %bottles_of_beer local
    print %on_the_wall local

    if %number_of_bottles local +1 +3
    print (text %iota local "") local

    return
.end

.function: bottles_of_beer/1
    .name: %iota total_number_of_bottles
    arg %total_number_of_bottles local %0

    ; display first three lines of a paragraph
    frame ^[(param %0 %total_number_of_bottles local)]
    call void first_print/1

    ; decrement the number of bottles
    idec %total_number_of_bottles local

    ; display last line of a paragraph
    frame ^[(param %0 %total_number_of_bottles local)]
    call void second_print/1

    ; immediately return if there are no more bottles
    if %total_number_of_bottles local theres_more +1
    return

    .mark: theres_more
    ; if there are more bottles
    ; call the function once more
    frame ^[(pamv %0 %total_number_of_bottles local)]
    tailcall bottles_of_beer/1
.end

.function: main/0
    .name: %iota total_number_of_bottles
    integer %total_number_of_bottles local 9

    frame ^[(pamv %0 %total_number_of_bottles local)]
    call void bottles_of_beer/1

    izero %0 local
    return
.end
