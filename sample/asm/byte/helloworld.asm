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

.function: main/1
    .name: 2 H
    .name: 3 e
    .name: 4 l
    .name: 5 o
    .name: 7 W
    .name: 8 r
    .name: 9 d

    ; exclamation mark
    .name: 1 _exmark
    bstore _exmark 33

    bstore H 72
    bstore e 101
    bstore l 108
    bstore o 111

    .name: 6 _space
    bstore _space 32

    bstore W 87
    bstore r 114
    bstore d 100

    echo H
    echo e
    echo l
    echo l
    echo o

    echo _space

    echo W
    echo o
    echo r
    echo l
    echo d

    print _exmark

    izero 0
    return
.end
