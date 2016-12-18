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

.function: foo/0
    print (istore 1 42)
    return
.end

.function: bar/0
    print (istore 1 69)
    frame 0
    call void foo/0
    return
.end

.function: baz/0
    print (istore 1 1995)
    frame 0
    call void bar/0
    return
.end

.function: bay/0
    print (istore 1 2015)
    frame 0
    call void baz/0
    return
.end


.function: main/1
    frame 0
    call void bay/0

    izero 0
    return
.end
