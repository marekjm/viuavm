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

.function: foo/0
    ; register 1 is captured from 'returns_closure' function
    print %1 local
    return
.end

.function: returns_closure/0
    closure %2 local foo/0
    capturecopy %2 local %1 (integer %1 local 42) local
    move %0 local %2 local
    return
.end

.function: main/1
    .name: 1 bar
    ; call function that returns the closure
    frame %0
    call %bar local returns_closure/0

    ; create frame for our closure and
    ; call it
    frame %0 %0
    call void %bar local

    izero %0 local
    return
.end
