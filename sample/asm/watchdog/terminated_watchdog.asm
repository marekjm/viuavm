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

.function: watchdog_process/0
    .mark: watchdog_start
    throw (remove 4 (arg 1 0) (strstore 3 "function"))

    ;frame ^[(param 0 (ptr 2 1)) (param 1 (strstore 3 "function"))]
    ;msg 4 get

    ;echo (strstore 5 "process spawned with <")
    ;echo 4
    ;print (strstore 5 "> died")

    ;jump watchdog_start

    return
.end

.function: broken_process/0
    watchdog watchdog_process/0

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
    nop
    throw (istore 1 42)
    return
.end

.function: main/1
    frame 0
    process void broken_process/0

    izero 0
    return
.end
