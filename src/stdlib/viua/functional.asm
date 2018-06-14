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

; This script implements basic functions used in functional programming style
;
; It can be dynamically linked to programs running on Viua VM by putting 'import "std::functional"' instruction in
; source code of the script.
; Alternatively, it can be statically linked if that would be more suitable.
;

.function: std::functional::filter/2
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
    if (gte %6 local %4 local %5 local) local loop_end loop_body

    .mark: loop_body

    ; call filtering function to determine whether current element
    ; is a valid value
    frame %1
    vat %7 local %2 local @4 local
    param %0 %7 local
    call %8 local %1 local

    ; if the result from filtering function was "true" - the element should be pushed onto result vector
    ; it it was "false" - skip to next iteration
    if %8 local element_ok next_iter

    .mark: element_ok
    vpush %3 local %7 local

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

.function: std::functional::foreach/2
    ; this function takes two mandatory parameters:
    ;
    ;       * a closure, or a function object to call,
    ;       * a vector of values that will be supplied as parameters to given callback,
    ;
    arg (.name: %iota callback) local %0
    arg (.name: %iota list) local %1

    ; setup loop counter and
    ; loop termination variable
    izero (.name: %iota counter) local
    vlen (.name: %iota list_length) local %list local

    ; loop condition
    .mark: loop_begin
    if (lt %iota local %counter local %list_length local) local loop_body loop_end

    .mark: loop_body

    ; extract parameter value
    vat (.name: %iota element) local %list local @counter local

    ; invoke given callback
    frame ^[(param %0 %element local)]
    call void %callback local

    iinc %counter local
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
    arg %1 local %0
    arg %2 local %1

    ; new vector to store mapped values
    vector %3 local

    ; set initial counter value and
    ; loop termination variable
    izero %4 local
    vlen %5 local %2 local

    ; while (...) {
    .mark: loop_begin
    gte %6 local %4 local %5 local
    if %6 local loop_end loop_body

    .mark: loop_body

    ; call supplied function on current element
    frame %1
    vat %7 local %2 local @4 local
    param %0 %7 local
    call %8 local %1 local

    ; push result to new vector
    vpush %3 local %8 local

    ; increase loop counter and go back to the beginning
    ;     ++i;
    ; }
    iinc %4 local
    jump loop_begin

    .mark: loop_end

    ; move vector with mapped values to the return register
    move %0 local %3 local
    return
.end

.block: std::functional::apply::__try_calling
    ; FIXME refactor this to use ^[] syntax
    frame %1
    param %0 %2 local
    call %3 local %1 local
    move %0 local %3 local
    leave
.end
.block: std::functional::apply::__catch
    ; just ignore the error if the function didn't return a value
    delete (draw %4 local) local
    leave
.end
; FIXME Remove the no_sa attribute after the new SA is able to handle this function.
; This will require implementing going down through entered blocks.
.function: [[no_sa]] std::functional::apply/2
    ; this function applies another function on a single parameter
    ;
    ; this function is type agnostic
    ; it just passes the parameter without additional processing
    .name: %1 func
    .name: %2 parameter

    ; extract the parameters
    arg %func local %0
    arg %parameter local %1

    ; apply the function to the parameter...
    ;frame %1
    ;param %0 parameter
    ;call %3 func
    try
    catch "Exception" std::functional::apply::__catch
    enter std::functional::apply::__try_calling

    return
.end

.function: [[no_sa]] std::functional::invoke/2
    ; this function takes two parameters:
    ;    1) a function object
    ;    2) a vector of parameters for function given as first parameter
    ;
    ; it then creates a frame with required number of parameter slots (as
    ; specified by length of the vector), and calls given function with this
    ; frame
    arg %1 local %0
    arg %2 local %1

    ; take length of the vector
    .name: %4 vector_length
    vlen %vector_length local %2 local
    frame @vector_length

    ; zero loop counter
    .name: %3 loop_counter
    izero %loop_counter local
    .mark: while_begin

    ; simple condition:
    ; while (loop_counter < vector_length) {
    .name: %5 loop_condition
    gte %loop_condition local %loop_counter local %vector_length local
    if %loop_condition local while_end while_body

    .mark: while_body

    ; store item located inside parameter vector at index denoted by loop_counter in
    ; a register
    .name: %7 slot
    vat %slot local %2 local @loop_counter local

    ; add parameter
    param @loop_counter %slot local

    ; loop_counter++
    iinc %loop_counter local

    jump while_begin

    .mark: while_end

    ; finally, after the frame is ready
    ; call the function
    call %8 local %1 local
    move %0 local %8 local
    return
.end
