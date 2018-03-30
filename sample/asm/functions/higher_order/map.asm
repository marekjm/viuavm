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

.function: square/1
    ; this function takes single integer as its argument,
    ; squares it and returns the result
    mul %0 local (arg %1 local %0) local %1 local
    return
.end

.function: apply/2
    ; this function applies another function on a single parameter
    ;
    ; this function is type agnostic
    ; it just passes the parameter without additional processing
    .name: 1 func
    .name: 2 parameter

    ; apply the function to the parameter...
    frame ^[(param %0 (arg %parameter local %1) local)]
    call %3 local (arg %func local %0) local

    ; ...and return the result
    move %0 local %3 local
    return
.end

.function: map/2
    ; this function takes two arguments:
    ;   * a function,
    ;   * a vector,
    ; then, it maps (i.e. calls) local the given function on every element of given vector
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

    ; while (...) local {
    .mark: loop_begin
    if (gte %6 local %4 local %5 local) local loop_end

    ; call supplied function on current element...
    frame ^[(param %0 *(vat %7 local %2 local %4 local) local)]
    ; ...and push result to new vector
    vpush %3 local (call %8 local %1 local) local

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

.function: main/1
    ; applies function square/1(int) local to 5 and
    ; prints the result

    vpush (vector %1 local) local (integer %2 local 1) local
    vpush %1 local (integer %2 local 2) local
    vpush %1 local (integer %2 local 3) local
    vpush %1 local (integer %2 local 4) local
    vpush %1 local (integer %2 local 5) local

    print %1 local

    frame ^[(param %0 (function %3 local square/1) local) (param %1 %1 local)]
    print (call %4 local map/2) local

    izero %0 local
    return
.end
