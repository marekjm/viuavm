;
;   Copyright (C) 2016, 2017 Marek Marecki
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

.block: pull_and_do_nothing
    draw %3 local
    leave
.end

.block: await_message
    idec %1 local
    receive %3 local 100ms
    leave
.end

.function: message_sender/2
    .name: %iota times
    .name: %iota pid
    arg %times local .iota: 0  %iota
    arg %pid local %1

    try
    catch "Exception" pull_and_do_nothing
    enter await_message

    if %times local next_iteration
    send %pid local (string %3 local "Hello World!") local
    return

    .mark: next_iteration
    frame ^[(pamv %iota %times local) (pamv %iota %pid local)]
    tailcall message_sender/2

    return
.end

.function: main/0
    frame ^[(pamv %iota (integer %1 local 5) local) (pamv %iota (self %1 local) local)]
    process void message_sender/2

    print (receive %iota local infinity) local

    izero %0 local
    return
.end
