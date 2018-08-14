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

; this example is the Viua assembly version of the following JS:
;
;   function closure_maker(x) {
;       function closure_level_1(a) {
;           function closure_level_2(b) {
;               function closure_level_3(c) {
;                   return (x + a + b + c);
;               }
;               return closure_level_3;
;           }
;           return closure_level_2;
;       }
;       return closure_level_1;
;   }
;
;   closure_maker(1)(2)(3)(4) == 10
;

.closure: closure_level_3/1
    ; expects 1, 2 and 3 to be captured integers
    .name: 5 accumulator
    move %4 local %0 parameters
    add %accumulator local %1 local %2 local
    add %accumulator local %3 local %accumulator local
    add %accumulator local %4 local %accumulator local
    move %0 local %accumulator local
    return
.end

.closure: closure_level_2/1
    closure %0 local closure_level_3/1
    ; registers 1 and 2 are occupied by captured integers
    ; but they must be captured by the "closure_level_3"
    capture %0 local %1 %1 local
    capture %0 local %2 %2 local
    capture %0 local %3 (move %3 local %0 parameters) local
    return
.end

.closure: closure_level_1/1
    closure %0 local closure_level_2/1
    ; register 1 is occupied by captured integer
    ; but it must be captured by the "closure_level_2"
    capture %0 local %1 %1 local
    capture %0 local %2 (move %2 local %0 parameters) local
    return
.end

.function: closure_maker/1
    allocate_registers %6 local

    ; create the outermost closure
    closure %0 local closure_level_1/1
    capture %0 local %1 (move %1 local %0 parameters) local
    return
.end

.function: main/1
    allocate_registers %6 local

    frame ^[(copy %0 arguments (integer %1 local 1) local)]
    call %2 local closure_maker/1

    frame ^[(copy %0 arguments (integer %1 local 2) local)]
    call %3 local %2 local

    frame ^[(copy %0 arguments (integer %1 local 3) local)]
    call %4 local %3 local

    frame ^[(copy %0 arguments (integer %1 local 4) local)]
    print (call %5 local %4 local) local

    izero %0 local
    return
.end
