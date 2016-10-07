;
;   Copyright (C) 2016 Marek Marecki
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
    pull 3
    leave
.end

.block: await_message
    idec 1
    receive 3 100ms
    leave
.end

.function: message_sender/2
    .name: iota times
    .name: iota pid
    arg times .iota: 0 iota
    arg pid 1

    try
    catch "Exception" pull_and_do_nothing
    enter await_message

    branch times next_iteration
    send pid (strstore 3 "Hello World!")
    return

    .mark: next_iteration
    frame ^[(pamv iota times) (pamv iota pid)]
    tailcall message_sender/2

    return
.end

.function: main/0
    frame ^[(pamv iota (istore 1 5)) (pamv iota (self 1))]
    process 0 message_sender/2

    print (receive iota infinity)

    izero 0
    return
.end
