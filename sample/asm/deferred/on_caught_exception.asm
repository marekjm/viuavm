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

.function: foo/0
    print (text %1 local "Hello foo World!") local
    return
.end

.function: bar/0
    print (text %1 local "Hello bar World!") local
    return
.end

.function: throwing/0
    frame %0
    defer foo/0

    frame %0
    defer bar/0

    throw (istore %1 local 42) local
    return
.end

.block: main__catch
    draw %2 local
    print %2 local
    leave
.end

.block: main__try
    frame %0
    call void throwing/0
    leave
.end

.function: main/0
    try
    catch "Integer" main__catch
    enter main__try

    izero %0 local
    return
.end
