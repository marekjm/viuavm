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

; This script implements basic functions used in functional programming style
;
; It can be dynamically linked to programs running on Viua VM by putting "link std::functional" instruction in
; source code of the script.
; Alternatively, it can be statically linked if that would be more suitable.
;

.function: std::functional::filter/2
    ; classic filter() function
    ; it takes two arguments:
    ;   * a filtering function,
    ;   * a vector with values to be filtered,
    arg 1 0
    arg 2 1

    ; vector for filtered values
    vec 3

    ; initial loop counter and
    ; loop termination variable
    izero 4
    vlen 5 2

    ; while (...) {
    .mark: loop_begin
    igte 6 4 5
    branch 6 loop_end loop_body

    .mark: loop_body

    ; call filtering function to determine whether current element
    ; is a valid value
    frame 1
    vat 7 2 @4
    param 0 7
    fcall 8 1

    ; if the result from filtering function was "true" - the element should be pushed onto result vector
    ; it it was "false" - skip to next iteration
    branch 8 element_ok next_iter

    .mark: element_ok
    vpush 3 7

    .mark: next_iter
    ; empty the register because vat instruction creates references
    empty 7

    ; increase the counter and go back to the beginning of the loop
    ;     ++i;
    ; }
    iinc 4
    jump loop_begin

    .mark: loop_end

    ; move result vector into return register
    move 0 3
    return
.end

.function: std::functional::foreach/2
    ; this function takes two mandatory parameters:
    ;
    ;       * a closure, or a function object to call,
    ;       * a vector of values that will be supplied as parameters to given callback,
    ;
    .name: 1 callback
    .name: 2 list
    arg callback 0
    arg list 1

    ; setup loop counter and
    ; loop termination variable
    .name: 3 counter
    izero counter
    vlen 4 list

    ; loop condition
    .mark: loop_begin
    ilt 5 3 4
    branch 5 loop_body loop_end

    .mark: loop_body

    ; extract parameter value
    vat 6 list @counter

    ; invoke given callback
    frame 1
    param 0 6
    fcall 7 callback

    ; empty register (required for VAT instruction)
    empty 6

    iinc counter
    jump loop_begin

    .mark: loop_end

    return
.end

.function: std::functional::map/2
    ; this function takes two arguments:
    ;   * a function,
    ;   * a vector,
    ; then, it maps (i.e. calls) the given function on every element of given vector
    ; and returns a vector containing modified values.
    ; returned vector is a newly created one - this function does not modify vectors in place.
    arg 1 0
    arg 2 1

    ; new vector to store mapped values
    vec 3

    ; set initial counter value and
    ; loop termination variable
    izero 4
    vlen 5 2

    ; while (...) {
    .mark: loop_begin
    igte 6 4 5
    branch 6 loop_end loop_body

    .mark: loop_body

    ; call supplied function on current element
    frame 1
    vat 7 2 @4
    param 0 7
    fcall 8 1

    ; push result to new vector
    vpush 3 8

    ; empty the register, as vat instruction creates references
    empty 7

    ; increase loop counter and go back to the beginning
    ;     ++i;
    ; }
    iinc 4
    jump loop_begin

    .mark: loop_end

    ; move vector with mapped values to the return register
    move 0 3
    return
.end

.block: std::functional::apply::__try_calling
    frame 1
    param 0 2
    fcall 3 1
    move 0 3
    leave
.end
.block: std::functional::apply::__catch
    ; just ignore the error if the function didn't return a value
    delete (pull 4)
    leave
.end
.function: std::functional::apply/2
    ; this function applies another function on a single parameter
    ;
    ; this function is type agnostic
    ; it just passes the parameter without additional processing
    .name: 1 func
    .name: 2 parameter

    ; extract the parameters
    arg func 0
    arg parameter 1

    ; apply the function to the parameter...
    ;frame 1
    ;param 0 parameter
    ;fcall 3 func
    try
    catch "Exception" std::functional::apply::__catch
    enter std::functional::apply::__try_calling

    return
.end

.function: std::functional::invoke/2
    ; this function takes two parameters:
    ;    1) a function object
    ;    2) a vector of parameters for function given as first parameter
    ;
    ; it then creates a frame with required number of parameter slots (as
    ; specified by length of the vector), and calls given function with this
    ; frame
    arg 1 0
    arg 2 1

    ; take length of the vector
    .name: 4 vector_length
    vlen vector_length 2
    frame @vector_length

    ; zero loop counter
    .name: 3 loop_counter
    izero loop_counter
    .mark: while_begin

    ; simple condition:
    ; while (loop_counter < vector_length) {
    .name: 5 loop_condition
    igte loop_condition loop_counter vector_length
    branch loop_condition while_end while_body

    .mark: while_body

    ; store item located inside parameter vector at index denoted by loop_counter in
    ; a register
    .name: 7 slot
    vat slot 2 @loop_counter

    ; add parameter
    param @loop_counter slot

    ; clear parameter slot
    empty slot

    ; loop_counter++
    iinc loop_counter

    jump while_begin

    .mark: while_end

    ; finally, after the frame is ready
    ; call the function
    fcall 8 1
    move 0 8
    return
.end
