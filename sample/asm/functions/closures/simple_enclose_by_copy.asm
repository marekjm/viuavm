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
    ; register 1 is captured from 'returns_closure' function
    print 1
    return
.end

.function: returns_closure/0
    closure 2 foo/0
    move 0 (capturecopy 2 1 (istore 1 42))
    return
.end

.function: main/1
    .name: 1 bar
    ; call function that returns the closure
    frame 0
    call bar returns_closure/0

    ; create frame for our closure and
    ; call it
    frame 0 0
    fcall 0 bar

    izero 0
    return
.end
