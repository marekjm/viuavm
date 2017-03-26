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

.function: factorial/2
    .name: 1 number
    .name: 0 result
    ; multiply "result" (reference) by "number" (copy)
    ; and store the resulting integer in "result"
    ; calculation is available outside of the local scope
    mul int64 %result (arg %result %1) (arg %number %0)
    idec %number

    ; if counter is equal to zero
    ; finish "factorial" calls
    if (eq int64 %4 %number (istore %3 0)) finish

    ; this frame must be the same as in "main"
    ; result must still be a reference
    frame ^[(pamv %0 %number) (pamv %1 %result)]
    tailcall factorial/2

    .mark: finish
    return
.end

.function: main/1
    .name: 1 number
    .name: 2 result
    ; store the number of which we want to calculate the factorial
    istore %number 8

    ; create frame for two parameters:
    ; * first is a copy of the number
    ; * second is a reference to result register
    ;   because we want to display it here, after calls to factorial are finished
    frame ^[(param %0 %number) (pamv %1 (istore %result 1))]
    call %result factorial/2

    ; print result
    print %result
    izero %0 local
    return
.end
