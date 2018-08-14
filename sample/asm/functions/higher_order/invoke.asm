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

.function: sum/4
    allocate_registers %5 local

    ; this function takes four integers as parameters and
    ; adds them, and returns the sum
    .name: %iota a
    .name: %iota b
    .name: %iota c
    .name: %iota d

    move %a local %0 parameters
    move %b local %1 parameters
    move %c local %2 parameters
    move %d local %3 parameters

    add %0 local %a local %b local  ; x = a + b
    add %0 local %0 local %c local  ; x = x + c
    add %0 local %0 local %d local  ; x = x + d

    return
.end

.function: invoke/2
    allocate_registers %9 local

    ; this function takes two parameters:
    ;    1) local a function object
    ;    2) local a vector of parameters for function given as first parameter
    ;
    ; it then creates a frame with required number of parameter slots (as
    ; specified by length of the vector) local, and calls given function with this
    ; frame
    .name: %iota fn_to_call
    .name: %iota parameters_list
    move %fn_to_call local %0 parameters
    move %parameters_list local %1 parameters

    ; take length of the vector
    .name: %iota vector_length
    ;vlen %vector_length local %2 local
    vlen %vector_length local %parameters_list local
    frame @vector_length local

    ; zero loop counter
    .name: %iota loop_counter
    izero %loop_counter local
    .mark: while_begin

    ; simple condition:
    ; while (loop_counter < vector_length) local {
    if (gte %iota local %loop_counter local %vector_length local) local while_end while_body

    .mark: while_body

    .name: %iota slot
    ; store item located inside parameter vector at index denoted by loop_counter in
    ; a register and
    ; pass it as a parameter
    vat %slot local %parameters_list local %loop_counter local
    move @loop_counter arguments (copy %iota local *slot local) local

    ; loop_counter++
    iinc %loop_counter local

    jump while_begin

    .mark: while_end

    ; finally, after the frame is ready
    ; call the function
    move %0 local (call %iota local %fn_to_call local) local
    return
.end

.function: main/1
    allocate_registers %9 local

    ; create the vector
    vpush (vector %1 local) local (integer %2 local 20) local
    vpush %1 local (integer %3 local 16) local
    vpush %1 local (integer %4 local 8) local
    vpush %1 local (integer %5 local -2) local

    integer %2 local 20
    integer %3 local 16
    integer %4 local 8
    integer %5 local -2

    ; call sum/4() local function
    frame ^[(copy %0 arguments %2 local) (copy %1 arguments %3 local) (copy %2 arguments %4 local) (copy %3 arguments %5 local)]
    print (call %6 local sum/4) local

    ; call sum/4 function via invoke/2 function
    frame ^[(copy %0 arguments (function %7 local sum/4) local) (copy %1 arguments %1 local)]
    print (call %8 local invoke/2) local

    izero %0 local
    return
.end
