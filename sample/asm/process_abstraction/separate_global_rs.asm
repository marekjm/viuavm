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

.function: global_printer/0
    ; switch to global register set
    ress global

    ; wait until a message arrives
    receive %2

    ; print contents of first register
    ; this should throw an exception because this process
    ; did not set any global registers
    print %1

    return
.end

.function: global_writer/2
    ; switch to global register set
    ress global

    ; put second parameter (whatever it is) in
    ; global register 1
    arg %1 %1

    ; switch to local register set
    ress local

    ; send message to printer process to trigger it to
    ; print contents of global register 1
    ; it should cause an exception
    .name: 1 printer_process_handle
    send (arg %printer_process_handle %0) (izero %3)

    return
.end

.signature: std::misc::cycle/1

.function: main/0
    import "std::misc"

    ; spawn printer process
    ; it immediately waits for a message to arrive
    ; first message it receives should crash it
    frame %0
    process %1 global_printer/0
    frame ^[(param %0 %1)]
    msg void detach/1

    ; spawn two independent writer processes
    ; whichever triggers the printer process is not important
    frame ^[(param %0 %1) (pamv %1 (strstore %2 "Hello World"))]
    process %2 global_writer/2
    frame ^[(param %0 %2)]
    msg void detach/1

    ; this is the second writer process
    frame ^[(param %0 %1) (pamv %1 (strstore %2 "broken"))]
    process %2 global_writer/2
    frame ^[(param %0 %2)]
    msg void detach/1

    izero %0 local
    return
.end
