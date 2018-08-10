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

.function: recursive/2
    allocate_registers %4 local

    .name: 1 counter
    .name: 2 zero

    ; unpack arguments
    arg %counter local %0
    arg %zero local %1

    ; decrease counter and check if it's less than zero
    if (lt %3 local (idec %counter local) local %zero local) break_rec
    print %counter local

    frame ^[(copy %0 arguments %counter local) (move %1 arguments %zero local)]
    call void recursive/2

    .mark: break_rec
    return
.end

.function: main/1
    allocate_registers %3 local

    ; create frame and set initial parameters
    frame ^[(copy %0 arguments (integer %1 local 10) local) (move %1 arguments (integer %2 local 0) local)]
    call void recursive/2

    izero %0 local
    return
.end
