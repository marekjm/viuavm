;
;  Copyright (C) 2018 Marek Marecki
;
;  This file is part of Viua VM.
;
;  Viua VM is free software: you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation, either version 3 of the License, or
;  (at your option) any later version.
;
;  Viua VM is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
;

.function: lazy_generator/1
    allocate_registers %4 local

    .name: iota value
    .name: iota next_value
    .name: iota respond_to

    ; Wait for a message to arrive.
    ; We assume that the message is the PID of the process to which the
    ; value should be sent back.
    ;
    ; Here, we wait for 1 second for a message to arrive.
    ; After this timeout, an exception is thrown and the process
    ; crashes. We could instead use:
    ;
    ;   receive %respond_to local infinity
    ;
    ; ...but then the process would wait indefinitely, and the VM would
    ; appear to have hanged. You could use ^C to kill it, but timing out
    ; if no requests are coming our way is just a little bit more
    ; elegant.
    receive %respond_to local 1s

    ; After we received a message, it makes sense to fetch the value
    ; with which we will respond.
    arg %value local %0

    ; Make a copy of the value and increase it by one. Each time we want
    ; to respond with a value that will be greater by one than the value
    ; sent last time. This will create the illusion of a lazily-created,
    ; infinite stream of ever increasing integers.
    copy %next_value local %value local
    iinc %next_value local

    ; Send the current value to the process that requested it.
    send %respond_to local %value local

    ; And perform a tail call to lazy_generator/1 to reinitialise it
    ; with new value. Tail recurion is an elegant (and efficient!) way
    ; to loop. It is also *much* more readable than a "normal" loop; at
    ; least when written in Viua VM assembly.
    frame %1
    move %0 arguments %next_value local
    tailcall lazy_generator/1
.end

.function: get_value/1
    allocate_registers %3 local

    .name: iota generator
    .name: iota this_process

    ; Send a request for new value to the generator.
    ; This means sending it our PID.
    arg %generator local %0
    self %this_process local
    send %generator local %this_process local
    receive %0 local infinity

    return
.end

.function: main/0
    allocate_registers %3 local

    .name: iota value
    .name: iota generator
    .name: iota this_process

    ; First, we need to initialise the generator. We can have many
    ; copies of the generator in-flight, and request values from them
    ; concurrently. It is safe since they run in different processes and
    ; do not share state with each other.
    integer %value local 42
    frame %1
    move %0 arguments %value local
    process %generator local lazy_generator/1

    ; Then, we can request values from the generator. It will generate
    ; them on demand, performing the role of an inifinite source of ever
    ; increasing integers.
    frame %1
    param %0 %generator local
    call %value local get_value/1
    print %value local

    frame %1
    param %0 %generator local
    call %value local get_value/1
    print %value local

    frame %1
    param %0 %generator local
    call %value local get_value/1
    print %value local

    frame %1
    param %0 %generator local
    call %value local get_value/1
    print %value local

    ; ...and again, and again, and again.

    izero %0 local
    return
.end
