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

.function: std::vector::of_ints/1
    ; Returns a vector of integers from 0 to N-1.
    ;
    ; N is received as first and only parameter
    ;
    allocate_registers %5 local

    .name: 1 limit
    move %limit %0 parameters

    .name: 0 vec
    vector %vec %3

    .name: 3 counter
    .name: 4 to_push
    izero %counter

    .mark: begin_loop
    vpush %vec (copy %to_push %counter)
    iinc %counter
    ; reuse 'to_push' register since it's empty
    if (gte %to_push %counter %limit) +1 begin_loop

    return
.end

.function: std::vector::of/2
    ; Returns a vector of N objects created by supplied function.
    ;
    ; N is received as first parameter.
    ; The spawning function is received as the second parameter.
    ;
    allocate_registers %5 local

    .name: 1 limit
    .name: 2 fn
    move %limit local %0 parameters
    move %fn local %1 parameters

    .name: 0 vec
    vector %vec local

    .name: 3 counter
    .name: 4 to_push
    izero %counter local

    .mark: begin_loop
    frame ^[(move %0 arguments (copy %to_push local %counter local) local)]
    vpush %vec local (call %to_push local %fn local) local
    iinc %counter local
    ; reuse 'to_push' register since it's empty
    if (gte %to_push local %counter local %limit local) local +1 begin_loop

    return
.end

.function: std::vector::reverse/1
    ; Returns reversed vector.
    ;
    ; This function expects to receive its parameter by copy.
    ; Vector passed as the parameter is emptied.
    ; Reversing is *NOT* performed in-place.
    ;
    allocate_registers %4 local

    .name: 1 source
    .name: 0 result
    move %source %0 parameters
    vector %result

    .name: 2 counter
    vlen %counter %source

    .mark: begin_loop
    .name: 3 tmp
    vpush %result (vpop %tmp %source)
    if (idec %counter) begin_loop
    .mark: end_loop

    return
.end

.function: std::vector::reverse_in_place/1
    ; Returns vector reversed in-place.
    ;
    ; This function expects to receive its parameter by move (to avoid
    ; copying).
    ;
    allocate_registers %5 local

    .name: 0 source
    move %source local %0 parameters

    .name: 1 counter_down
    vlen %counter_down local %source local
    idec %counter_down local
    .name: 2 counter_up
    izero %counter_up local
    .name: 3 limit
    copy %limit local %counter_down local

    .mark: begin_loop
    .name: 4 tmp
    vpop %tmp local %source local
    vinsert %source local %tmp local %counter_up local

    idec %limit local
    idec %counter_down local
    iinc %counter_up local

    if %limit local begin_loop
    .mark: end_loop

    return
.end

.function: std::vector::every/2
    ; Returns true if every element of the vector passes a test (supplied as a function in second parameter), false otherwise.
    ;
    allocate_registers %7 local

    .name: 1 vec
    .name: 2 fn
    move %vec %0 parameters
    move %fn %1 parameters

    .name: 0 result
    not (izero %result)

    .name: 3 limit
    .name: 4 index
    .name: 5 tmp
    vlen %limit %vec
    izero %index

    ; do not loop on zero-length vectors
    if %limit +1 end_loop
    .mark: begin_loop
    vat %tmp %vec %index
    ; FIXME: there should be no copy operation - use pass-by-move instead
    frame ^[(copy %0 arguments *tmp local)]
    and %result (call %6 %fn) %result

    ; break loop if there wasn't a match
    if %result +1 end_loop

    if (gte %tmp (iinc %index) %limit) +1 begin_loop
    .mark: end_loop

    return
.end

.function: std::vector::any/2
    ; Returns true if every element of the vector passes a test (supplied as a function in second parameter), false otherwise.
    ;
    allocate_registers %7 local

    .name: 1 vec
    .name: 2 fn
    move %vec %0 parameters
    move %fn %1 parameters

    .name: 0 result
    not (izero %result)

    .name: 3 limit
    .name: 4 index
    .name: 5 tmp
    vlen %limit %vec
    izero %index

    ; do not loop on zero-length vectors
    if %limit +1 end_loop
    .mark: begin_loop
    vat %tmp %vec %index
    ; FIXME: there should be no copy operation - use pass-by-move instead
    frame ^[(copy %0 arguments *tmp local)]
    move %result (call %6 %fn)

    ; break the loop if there was a match
    if %result end_loop

    if (gte %tmp (iinc %index) %limit) +1 begin_loop
    .mark: end_loop

    return
.end
