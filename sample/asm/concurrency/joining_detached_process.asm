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

.function: running_detached/0
.name: 0 counter
.name: 2 limit
    izero %counter
    istore %limit 4
    strstore %1 "Hello World! (from long-running detached process) "
.mark: loop
    if (gte int64 %3 %counter %limit) after_loop
    echo %1
    print %counter
    iinc %counter
    jump loop
.mark: after_loop
    return
.end

.function: main/1
    frame %0
    process %1 running_detached/0

    nop
    nop

    frame ^[(param %0 (ptr %2 %1))]
    msg void detach/1

    nop

    ; reuse pointer creater earlier
    frame ^[(param %0 %2)]
    msg %3 joinable/1
    print %3

    ; this throws, cannot join detached process
    join %0 %1

    print (strstore %3 "main/1 exited")

    izero %0 local
    return
.end
