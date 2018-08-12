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
    move %1 local %0 parameters

    ; Here, the inferencer kicks in.
    ; SA sees that the value in the register is accessed using the by-pointer
    ; dereference mode, and deduces that the register must hold a pointer, and
    ; since the print instruction accepts any type, the pointerness of the value
    ; is the only assumption the inferencer can make.
    ;
    ; So, the deduced type is 'pointer to value'.
    print *1 local

    integer %2 local 42

    ; The SA will throw an error here.
    ; Value in register one is accessed with the assumption that it contains a
    ; vector which does not hold, given the usage of register one seen
    ; before indicating that the register holds a pointer to some value.
    ;
    ; Should the register be accessed with '*1' the error would not be thrown, and
    ; the type of the value would be further deduced to be 'pointer to vector' which
    ; would be a refinement of 'pointer to value'.
    ; See test 'sample/static_analysis/two_stage_pointerness_inference.asm' for an
    ; exapmle of this "refinement" feature.
    vinsert %1 local %2 local void

    izero %0 local
    return
.end
