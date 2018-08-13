;
;   Copyright (C) 2015, 2016, 2017, 2018 Marek Marecki
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

.function: misc::boolean/1
    ; this function returns boolean value of its parameter
    allocate_registers %1 local

    not (not (move %0 local %0 parameters) local) local
    return
.end

.function: std::misc::cycle/1
    ; executes at least N cycles
    ;
    allocate_registers %4 local

    move (.name: %iota counter) local %0 parameters

    .name: iota i
    sub %counter local %counter local (integer %i local 9) local
    div %counter local %counter local (integer %i local 2) local

    izero (.name: %iota zero) local

    .mark: __loop_begin
    if (lte %i local %counter local %zero local) local __loop_end +1
    idec %counter local
    jump __loop_begin
    .mark: __loop_end

    return
.end
