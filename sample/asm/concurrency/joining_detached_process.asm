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

.function: running_detached/1
    .name: %iota counter
    .name: %iota limit
    izero %counter local
    integer %limit local 4

    send (arg %iota local %0) local (self %iota local) local

    .name: %iota message
    string %message local "Hello World! (from long-running detached process) "

    .mark: loop
    if (gte %iota local %counter local %limit local) local after_loop
    echo %message local
    print %counter local
    iinc %counter local
    jump loop
    .mark: after_loop

    return
.end

.function: main/1
    frame ^[(pamv %0 (self %1 local) local)]
    process void running_detached/1

    receive %1 local

    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    ; this throws, cannot join detached process
    join void %1 local

    print (string %3 local "main/1 exited") local

    izero %0 local
    return
.end
