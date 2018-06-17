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

.function: main/1
    .name: 1 zero
    .name: 2 one

    ; store zero and one in registers
    integer %zero local 0
    integer %one local 1

    ; swap objects so that register "zero" now contains 1, and
    ; register "one" contains 0
    swap %zero local %one local

    ; print 0 and 1
    ; this should actually print 1 and 0
    print %zero local
    print %one local

    izero %0 local
    return
.end
