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

.function: run_in_a_process/1
    ; send our PID back to parent
    send (arg %iota local %0) local (self %iota local) local

    print (receive %iota local 10s) local
    return
.end

.function: main/1
    .name: %iota pid
    frame ^[(pamv %0 (self %pid local) local)]
    process void run_in_a_process/1

    receive %pid local 10s

    send %pid local (string %iota local "Hello message passing World!") local

    izero %0 local
    return
.end
