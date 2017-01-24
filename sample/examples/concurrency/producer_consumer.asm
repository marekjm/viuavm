;
;   Copyright (C) 2015, 2016 Marek Marecki
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

.function: consumer/0
    print (receive %2)
    print (strstore %1 "consumer/0: exiting")
    return
.end

.signature: std::io::getline/0

.function: producer/1
    ; the producer will read users input and
    ; send it to the consumer
    frame %0
    call %2 std::io::getline/0

    print (strstore %1 "producer/1: exiting")

    frame ^[(param %0 (arg %3 %0)) (param %1 %2)]
    msg void pass/2

    return
.end

.function: main/1
    import "io"

    ; spawn the consumer process...
    frame %0
    process %1 consumer/0

    ; ...and detach it, so main/1 exiting will not
    ; kill it
    frame ^[(param %0 (ptr %2 %1))]
    msg void detach/1

    ; spawn the producer process and pass consumer process handle to it
    ; producer needs to know what consumer it must pass a message to
    ;
    ; pass-by-move to avoid copying, the handle will not be used by main/1 at
    ; any later point
    frame ^[(pamv %0 %1)]
    process void producer/1

    ; main/1 may safely exit
    izero %0 local
    return
.end
