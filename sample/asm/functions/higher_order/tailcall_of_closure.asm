;
;   Copyright (C) 2017 Marek Marecki
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

.closure: closure/0
    throw %1 local
    return
.end

.function: test/0
    .name: %iota a_closure
    .name: %iota an_int

    integer %an_int local 42

    closure %a_closure local closure/0
    capturemove %a_closure local %1 %an_int local

    frame %0
    tailcall %a_closure local
.end

.function: main/0
    frame %0
    call void test/0

    izero %0 local
    return
.end
