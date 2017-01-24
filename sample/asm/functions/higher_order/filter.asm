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

.function: is_divisible_by_2/1
    arg %1 %0
    istore %2 2

    .mark: loop_begin
    if (lt int64 %3 %1 %2) loop_end +1

    sub int64 %1 %1 %2
    jump loop_begin

    ; make zero "true" and
    ; non-zero values "false"
    .mark: loop_end
    not (move %0 %1)

    return
.end

.function: filter/2
    ; classic filter() function
    ; it takes two arguments:
    ;   * a filtering function,
    ;   * a vector with values to be filtered,
    arg %1 %0
    arg %2 %1

    ; vector for filtered values
    vec %3

    ; initial loop counter and
    ; loop termination variable
    izero %4
    vlen %5 %2

    ; while (...) {
    .mark: loop_begin
    if (gte int64 %6 %4 %5) loop_end +1

    ; call filtering function to determine whether current element
    ; is a valid value...
    frame ^[(param %0 *(vat %7 %2 @4))]

    ; ...and if the result from filtering function was "true" - the element should be pushed onto result vector
    ; it it was "false" - skip to next iteration
    if (fcall %8 %1) element_ok next_iter

    .mark: element_ok
    vpush %3 *7

    .mark: next_iter

    ; increase the counter and go back to the beginning of the loop
    ;     ++i;
    ; }
    iinc %4
    jump loop_begin

    .mark: loop_end

    ; move result vector into return register
    move %0 %3
    return
.end

.function: main/1
    vpush (vec %1) (istore %2 1)
    vpush %1 (istore %2 2)
    vpush %1 (istore %2 3)
    vpush %1 (istore %2 4)
    vpush %1 (istore %2 5)

    print %1

    frame ^[(param %0 (function %3 is_divisible_by_2/1)) (param %1 %1)]
    print (call %4 filter/2)

    izero %0 local
    return
.end
