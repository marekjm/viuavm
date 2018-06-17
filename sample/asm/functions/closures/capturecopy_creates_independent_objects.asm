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

.function: a_closure/0
    ; expects register 1 to be captured
    print %1 local
    return
.end

.function: main/1
    string %1 local "Hello World!"

    closure %2 local a_closure/0
    capturecopy %2 local %1 %1 local

    print %1 local
    ; call the closure
    frame %0
    call void %2 local

    ; this should not affect the object captured a "a_closure"
    print (integer %1 local 42) local

    ; call the closure
    frame %0
    call void %2 local

    izero %0 local
    return
.end
