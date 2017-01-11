;
;   Copyright (C) 2016 Marek Marecki
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

.function: add/2
    arg (.name: %iota lhs) %0
    arg (.name: %iota rhs) %0

    add int64 %0 %lhs %rhs

    return
.end

.function: multiply/2
    arg (.name: %iota lhs) %0
    arg (.name: %iota rhs) %0

    mul int64 %0 %lhs %rhs

    return
.end

.function: main/0
    function (.name: %iota adder) add/2
    function (.name: %iota multipler) multiply/2

    istore (.name: %iota one) 1

    .name: %iota pointer
    .name: %iota result

    ptr %pointer %adder
    frame ^[(param %iota %one) (param %iota %one)]
    print (fcall %result *pointer)

    ptr %pointer %multipler
    frame ^[(param %iota %one) (param %iota %one)]
    print (fcall %result *pointer)

    izero %0
    return
.end
