;
;   Copyright (C) 2016, 2017, 2018 Marek Marecki
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
    allocate_registers %3 local

    move (.name: %iota lhs) local %0 parameters
    move (.name: %iota rhs) local %1 parameters

    add %0 local %lhs local %rhs local

    return
.end

.function: multiply/2
    allocate_registers %3 local

    move (.name: %iota lhs) local %0 parameters
    move (.name: %iota rhs) local %1 parameters

    mul %0 local %lhs local %rhs local

    return
.end

.function: main/0
    allocate_registers %6 local

    function (.name: %iota adder) local add/2
    function (.name: %iota multipler) local multiply/2

    integer (.name: %iota one) local 1

    .name: %iota pointer
    .name: %iota result

    ptr %pointer local %adder local
    frame ^[(copy %iota arguments %one local) (copy %iota arguments %one local)]
    print (call %result local *pointer local) local

    ptr %pointer local %multipler local
    frame ^[(copy %iota arguments %one local) (copy %iota arguments %one local)]
    print (call %result local *pointer local) local

    izero %0 local
    return
.end
