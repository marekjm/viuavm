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

; purpose of this program is to compute n-th power of a number

.function: dummy/0
    ; this function is here to stress jump calculation
    izero %0 local
    return
.end

.function: power_of/2
    .name: 1 base
    .name: 2 exponent
    .name: 3 zero
    .name: 6 result

    ; store operands of the power-of operation
    arg %base local %0

    ; if the exponent is equal to zero, store 1 in first register and jump to print
    ; invert so short form of branch instruction can be used
    if (not (eq %4 local (arg %exponent local %1) local (izero %zero local) local) local) local algorithm +1
    integer %result local 1
    jump final

    ; now, we multiply in a loop
    .mark: algorithm
    .name: 5 counter
    integer %counter local 1
    ; in register 6, store the base of power as
    ; we will need it for multiplication
    copy %result local %base local

    .mark: loop
    if (lt %4 local %counter local %exponent local) local +1 final
    mul %result local %result local %base local
    nop
    iinc %counter local
    jump loop

    ; final instructions
    .mark: final
    ; return result
    move %0 local %result local
    return
.end

.function: main/1
    frame ^[(param %0 (integer %1 local 4) local) (param %1 (integer %2 local 3) local)]
    print (call %1 local power_of/2) local

    izero %0 local
    return
.end
