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

; This script tests whether floats can be freely used as conditions for branch instruction.
; Basically, it justs tests correctness of the .boolean() method override in Integer objects.

.function: main/1
    float %1 local 0.0001

    ; generate false
    integer %2 local 0
    integer %3 local 1
    eq %2 local %2 local %3 local

    ; check
    if %1 local ok fin
    .mark: ok
    not %2 local
    .mark: fin
    print %2 local
    izero %0 local
    return
.end
