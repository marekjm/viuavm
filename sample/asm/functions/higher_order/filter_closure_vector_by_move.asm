;
;   Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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

.closure: is_divisible_by/1
    .name: 2 bound_variable
    move %1 local %0 parameters

    .mark: loop_begin
    if (lt %3 local %1 local %bound_variable local) local loop_end loop_body

    .mark: loop_body
    sub %1 local %1 local %bound_variable local
    jump loop_begin

    .mark: loop_end

    ; make zero "true" and
    ; non-zero values "false"
    ; FIXME: find out why `not (move 0 1) local` causes memory leak
    not (copy %0 local %1 local) local
    return
.end

.function: is_divisible_by_2/0
    allocate_registers %4 local

    closure %1 local is_divisible_by/1
    capturemove %1 local %2 (integer %2 local 2) local
    move %0 local %1 local
    return
.end

.function: filter_closure/2
    ; classic filter() local function
    ; it takes two arguments:
    ;   * a filtering function,
    ;   * a vector with values to be filtered,
    allocate_registers %9 local

    move %1 local %0 parameters
    move %2 local %1 parameters

    ; vector for filtered values
    vector %3 local

    ; initial loop counter and
    ; loop termination variable
    izero %4 local
    vlen %5 local %2 local

    ; while (...) local {
    .mark: loop_begin
    if (gte %6 local %4 local %5 local) local loop_end

    ; call filtering function to determine whether current element
    ; is a valid value
    frame ^[(copy %0 arguments *(vat %7 local %2 local %4 local) local)] %0
    call %8 local %1 local

    ; if the result from filtering function was "true" - the element should be pushed onto result vector
    ; it it was "false" - skip to next iteration
    if %8 local element_ok next_iter

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
    allocate_registers %5 local

    vpush (vector %1 local) local (integer %2 local 1) local
    vpush %1 local (integer %2 local 2) local
    vpush %1 local (integer %2 local 3) local
    vpush %1 local (integer %2 local 4) local
    vpush %1 local (integer %2 local 5) local

    print %1 local

    frame %0
    call %3 local is_divisible_by_2/0

    frame ^[(copy %0 arguments %3 local) (move %1 arguments %1 local)]
    print (call %4 local filter_closure/2) local

    izero %0 local
    return
.end
