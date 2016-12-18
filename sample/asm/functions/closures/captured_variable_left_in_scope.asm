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

.function: printer_function/0
    ; expects register 1 to be an captured object
    print 1
    return
.end

.function: main/1
    ; create a closure and capture object in register 1 with it
    closure 2 printer_function/0
    capture 2 1 (strstore 1 "Hello World!")

    ; call the closure (should print "Hello World!")
    frame 0
    fcall void 2

    ; store 42 in register 1, keep in mind that register 1 holds a reference so
    ; the istore will rebind the reference - it will now point to Integer(42)
    istore 1 42

    ; call the closure (should print "42")
    frame 0
    fcall void 2

    izero 0
    return
.end
