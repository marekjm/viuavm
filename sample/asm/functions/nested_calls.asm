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

.function: foo/0
    print (integer %1 local 42) local
    return
.end

.function: bar/0
    print (integer %1 local 69) local
    frame %0
    call void foo/0
    return
.end

.function: baz/0
    print (integer %1 local 1995) local
    frame %0
    call void bar/0
    return
.end

.function: bay/0
    print (integer %1 local 2015) local
    frame %0
    call void baz/0
    return
.end


.function: main/1
    frame %0
    call void bay/0

    izero %0 local local
    return
.end
