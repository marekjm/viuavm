;
;   Copyright (C) 2017, 2018 Marek Marecki <marekjm@ozro.pw>
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

.function: main/0
    allocate_registers %4 local

    bits (.name: %iota lhs) local 0b01111111
    bits (.name: %iota rhs) local 0b00000010

    bitnot %lhs local %lhs local
    checkedsincrement %lhs local

    bitnot %rhs local %rhs local
    checkedsincrement %rhs local

    checkedsadd (.name: %iota result) local %lhs local %rhs local

    print %lhs local
    print %rhs local
    print %result local

    izero %0 local
    return
.end
