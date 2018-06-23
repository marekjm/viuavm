;
;   Copyright (C) 2017, 2018 Marek Marecki <marekjm@ozro.pw>
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

.function: main/1
    allocate_registers %3 local

    ; An argument is extracted.
    ; At this point the SA is unable to make any assumptions about
    ; the type of the value contained in the register.
    arg %1 local %0

    ; Here, the inferencer kicks in for the first time.
    ; SA sees that the value in the register is accessed using the by-pointer
    ; dereference mode, and deduces that the register must hold a pointer, and
    ; since the print instruction accepts any type, the pointerness of the value
    ; is the only assumption the inferencer can make.
    ;
    ; So, the deduced type is 'pointer to value'.
    print *1 local

    ; Here is the second time the inferencer kicks in.
    ; SA sees that the value is accessed as the target of the iinc instruction, so
    ; it must be of type 'integer'.
    ; The value is also accessed using by-pointer mode, so the immediate value held
    ; in the register is a pointer (this agrees with the previous assumption).
    ; With this information, the inferencer fills missing type information and
    ; deduces the type of value to be 'pointer to integer'.
    iinc *1 local

    integer %2 local 42

    ; The SA will throw an error here.
    ; Value in register one is accessed with the assumption that it contains a
    ; pointer to vector which does not hold, given the usage of register one seen
    ; before.
    vinsert *1 local %2 local void

    izero %0 local
    return
.end
