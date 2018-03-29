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

.function: misc::boolean/1
    ; this function returns boolean value of its parameter
    not (not (arg %0 %0))
    return
.end

.function: std::misc::cycle/1
    ; executes at least N cycles
    ;
    arg (.name: %iota counter) %0

    .name: iota i
    sub %counter %counter (integer %i 9)
    div %counter %counter (integer %i 2)

    izero (.name: %iota zero)

    .mark: __loop_begin
    if (lte %i %counter %zero) __loop_end +1
    idec %counter
    jump __loop_begin
    .mark: __loop_end

    return
.end
