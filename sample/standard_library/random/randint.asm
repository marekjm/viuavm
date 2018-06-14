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

; std::random::randint/2(Integer lower_bound, Integer upper_bound) -> Integer
;   Returned integer is between lower and upper bound.
.signature: std::random::randint

; SYNOPSIS
;
;   viua-cpu a.out [lower-bound[, upper-bound]]
;
; DESCRIPTION
;
;   Program returns a random integer between upper and lower bound.
;   First given operand sets lower bound, second sets the upper one.
;   Default range is [0, 100).
;
; EXAMPLES
;
;   Return random integer between 0 and 100:
;
;       $ viua-cpu a.out
;       42
;
;   Return random integer between 42 and 127:
;
;       $ viua-cpu a.out 42 127
;       69
;
;   Return random integer between 40 and 100:
;
;       $ viua-cpu a.out 40
;       42
;
.function: main/1
    ; first, import the random module to make std::random functions available
    import "random"

    ; extract commandline operands vector
    arg %1 local %0
    vlen %2 local %1 local

    ; extract lower bound if 2 or more operands were given, default to 0 otherwise
    integer %3 local 2
    if (gte %5 local %2 local %3 local) local +1 default_lower_bound
    idec %3 local
    stoi %4 local *(vat %3 local %1 local @3 local) local
    ; register holding object obtained from vector must be emptied before reuse
    move %3 local %4 local
    ; jump two instructions to skip default lower bound assignment
    jump +2

    .mark: default_lower_bound
    integer %3 local 0

    ; extract upper bound if 3 or more operands were given, default to 100 otherwise
    integer %4 local 3
    if (gte %5 local %2 local %4 local) local +1 default_upper_bound
    idec %4 local
    stoi %5 local *(vat %4 local %1 local @4 local) local
    ; register holding object obtained from vector must be emptied before reuse
    move %4 local %5 local
    ; jump two instructions to skip default upper bound assignment
    jump +2

    .mark: default_upper_bound
    integer %4 local 100

    ; random integer between lower and upper bound
    frame ^[(param %0 %3 local) (param %1 %4 local)]
    print (call %6 local std::random::randint) local

    izero %0 local
    return
.end
