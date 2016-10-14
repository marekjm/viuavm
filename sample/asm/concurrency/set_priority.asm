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

.function: run_in_a_process/0
.name: 1 counter
.name: 2 limit
    izero counter
    istore limit 32
.mark: loop
    if (igte 4 counter limit) endloop +1
    ; execute nops to make the process longer
    nop
    iinc counter
    jump loop
.mark: endloop

    print (strstore 5 "Hello concurrent World!")
    return
.end

.function: main/1
    frame 0
    process 1 run_in_a_process/0

    frame ^[(param 0 1) (param 1 (istore 2 40))]
    msg 0 setPriority/2

    frame ^[(param 0 1)]
    print (msg 2 getPriority/1)
.name: 3 counter
.name: 4 limit
    izero counter
    ; set lower limit than for spawned process to force
    ; the priority settings take effect
    istore limit 16
.mark: loop
    if (igte 5 counter limit) endloop +1
    ; execute nops to make main process do something
    nop
    iinc counter
    jump loop
.mark: endloop

    print (strstore 6 "Hello sequential World!")

    join 0 1

    izero 0
    return
.end
