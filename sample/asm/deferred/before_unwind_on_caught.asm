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

.function: print_me/1
    print *(arg %1 local %0) local
    return
.end

.function: throwing/0
    throw (integer %1 local 666) local
    return
.end

.function: unfortunate/0
    text %1 local "Hello World before stack unwinding!"

    frame ^[(pamv %0 (ptr %2 local %1 local) local)]
    defer print_me/1

    frame %0
    call void throwing/0

    return
.end

.function: main/0
    try
    catch "Integer" .block: catch_Integer
        print (draw %2 local) local
        leave
    .end
    enter .block: try_this
        frame %0
        call void unfortunate/0
        leave
    .end

    izero %0 local
    return
.end
