;
;   Copyright (C) 2017, 2018 Marek Marecki
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

.function: deferred/0
    allocate_registers %2 local

    print (text %1 local "Hello from deferred!") local
    return
.end

.function: bar/0
    allocate_registers %2 local

    print (integer %1 local 42) local
    return
.end

.function: foo/0
    allocate_registers %0 local

    frame %0
    defer deferred/0

    frame %0
    tailcall bar/0
.end

.function: main/0
    allocate_registers %1 local

    frame %0
    call foo/0

    izero %0 local
    return
.end
