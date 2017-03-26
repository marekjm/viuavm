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

.signature: throwing::oh_noes/0

.block: __try
    frame %0
    call void throwing::oh_noes/0
    leave
.end
.block: __catch_Exception
    print (draw %1)
    leave
.end

.function: watchdog_process/0
    arg (.name: %iota death_message) %0

    .name: %iota exception
    remove %exception %death_message (strstore %exception "exception")
    print %exception

    return
.end

.function: main/1
    watchdog watchdog_process/0

    import "build/test/throwing"

    try
    catch "ExceptionX" __catch_Exception
    enter __try

    izero %0 local
    return
.end
