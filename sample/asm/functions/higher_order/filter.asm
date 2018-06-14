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

.function: is_divisible_by_2/1
    arg %1 local %0
    integer %2 local 2

    .mark: loop_begin
    if (lt %3 local %1 local %2 local) local loop_end +1

    sub %1 local %1 local %2 local
    jump loop_begin

    ; make zero "true" and
    ; non-zero values "false"
    .mark: loop_end
    not (move %0 local %1 local) local

    return
.end

.function: filter/2
    ; classic filter() function
    ; it takes two arguments:
    ;   * a filtering function,
    ;   * a vector with values to be filtered,
    arg %1 local %0
    arg %2 local %1

    ; vector for filtered values
    vector %3 local

    ; initial loop counter and
    ; loop termination variable
    izero %4 local
    vlen %5 local %2 local

    ; while (...) {
    .mark: loop_begin
    if (gte %6 local %4 local %5 local) local loop_end +1

    ; call filtering function to determine whether current element
    ; is a valid value...
    frame ^[(param %0 *(vat %7 local %2 local %4) local)]

    ; ...and if the result from filtering function was "true" - the element should be pushed onto result vector
    ; it it was "false" - skip to next iteration
    if (call %8 local %1 local) local element_ok next_iter

    .mark: element_ok
    vpush %3 local *7 local

    .mark: next_iter

    ; increase the counter and go back to the beginning of the loop
    ;     ++i;
    ; }
    iinc %4 local
    jump loop_begin

    .mark: loop_end

    ; move result vector into return register
    move %0 local %3 local
    return
.end

.function: main/1
    vpush (vector %1 local) local (integer %2 local 1) local
    vpush %1 local (integer %2 local 2) local
    vpush %1 local (integer %2 local 3) local
    vpush %1 local (integer %2 local 4) local
    vpush %1 local (integer %2 local 5) local

    print %1 local

    frame ^[(param %0 (function %3 local is_divisible_by_2/1) local) (param %1 %1 local)]
    print (call %4 local filter/2) local

    izero %0 local
    return
.end
