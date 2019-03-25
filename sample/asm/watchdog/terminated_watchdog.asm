;
;   Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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

.function: [[no_sa]] watchdog_process/1
    allocate_registers %5 local

    .mark: watchdog_start
    throw (structremove %4 local (move %1 local %0 parameters) local (atom %3 local 'function') local) local

    return
.end

.function: broken_process/0
    allocate_registers %2 local

    frame %1
    watchdog watchdog_process/1

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
    throw (integer %1 local 42) local
    return
.end

.function: main/1
    allocate_registers %1 local

    frame %0
    process void broken_process/0

    izero %0 local
    return
.end
