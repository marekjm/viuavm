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

.function: std::vector::of_ints/1
    ; Returns a vector of integers from 0 to N-1.
    ;
    ; N is received as first and only parameter
    ;
    .name: 1 limit
    arg limit 0

    .name: 0 vector
    vec vector

    .name: 3 counter
    .name: 4 to_push
    izero counter

    .mark: begin_loop
    vpush vector (copy to_push counter)
    iinc counter
    ; reuse 'to_push' register since it's empty
    if (gte int64 to_push counter limit) +1 begin_loop

    return
.end

.function: std::vector::of/2
    ; Returns a vector of N objects created by supplied function.
    ;
    ; N is received as first parameter.
    ; The spawning function is received as the second parameter.
    ;
    .name: 1 limit
    .name: 2 fn
    arg limit 0
    arg fn 1

    .name: 0 vector
    vec vector

    .name: 3 counter
    .name: 4 to_push
    izero counter

    .mark: begin_loop
    frame ^[(pamv 0 (copy to_push counter))]
    vpush vector (fcall to_push fn)
    iinc counter
    ; reuse 'to_push' register since it's empty
    if (gte int64 to_push counter limit) +1 begin_loop

    return
.end

.function: std::vector::reverse/1
    ; Returns reversed vector.
    ;
    ; This function expects to receive its parameter by copy.
    ; Vector passed as the parameter is emptied.
    ; Reversing is *NOT* performed in-place.
    ;
    .name: 1 source
    .name: 0 result
    arg source 0
    vec result

    .name: 2 counter
    vlen counter source

    .mark: begin_loop
    .name: 3 tmp
    vpush result (vpop tmp source)
    if (idec counter) begin_loop
    .mark: end_loop

    return
.end

.function: std::vector::reverse_in_place/1
    ; Returns vector reversed in-place.
    ;
    ; This function expects to receive its parameter by move (to avoid
    ; copying).
    ;
    .name: 0 source
    arg source 0

    .name: 1 counter_down
    vlen counter_down source
    idec counter_down
    .name: 2 counter_up
    izero counter_up
    .name: 3 limit
    copy limit counter_down

    .mark: begin_loop
    .name: 4 tmp
    vpop tmp source
    vinsert source tmp @counter_up

    idec limit
    idec counter_down
    iinc counter_up

    if limit begin_loop
    .mark: end_loop

    return
.end

.function: std::vector::every/2
    ; Returns true if every element of the vector passes a test (supplied as a function in second parameter), false otherwise.
    ;
    .name: 1 vector
    .name: 2 fn
    arg vector 0
    arg fn 1

    .name: 0 result
    not (izero result)

    .name: 3 limit
    .name: 4 index
    .name: 5 tmp
    vlen limit vector
    izero index

    ; do not loop on zero-length vectors
    if limit +1 end_loop
    .mark: begin_loop
    vpop tmp vector @index
    ; FIXME: there should be no copy operation - use pass-by-move instead
    frame ^[(param 0 tmp)]
    and result (fcall 6 fn) result

    vinsert vector tmp @index

    ; break loop if there wasn't a match
    if result +1 end_loop

    if (gte int64 tmp (iinc index) limit) +1 begin_loop
    .mark: end_loop

    return
.end

.function: std::vector::any/2
    ; Returns true if every element of the vector passes a test (supplied as a function in second parameter), false otherwise.
    ;
    .name: 1 vector
    .name: 2 fn
    arg vector 0
    arg fn 1

    .name: 0 result
    not (izero result)

    .name: 3 limit
    .name: 4 index
    .name: 5 tmp
    vlen limit vector
    izero index

    ; do not loop on zero-length vectors
    if limit +1 end_loop
    .mark: begin_loop
    vpop tmp vector @index
    ; FIXME: there should be no copy operation - use pass-by-move instead
    frame ^[(param 0 tmp)]
    move result (fcall 6 fn)

    ; put the value back into the vector
    vinsert vector tmp @index

    ; break the loop if there was a match
    if result end_loop

    if (gte int64 tmp (iinc index) limit) +1 begin_loop
    .mark: end_loop

    return
.end
