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

.function: [[no_sa]] counter/1
    allocate_registers %5 local

    ; if register 1 is *not null* jump to increase marker
    ; otherwise continue execution to perform initial set up of static registers
    isnull %2 local %1 static
    not %2 local %2 local
    if %2 local increase

    ; these instructions are executed only when 1 register was null
    ; they first setup static counter variable
    integer %1 static 0

    ; 1) fake taking counter from static registers (it's zero during first pass anyway)
    integer %1 local 0
    ; 2) fetch the argument
    move %3 local %0 parameters
    ; 3) jump straight to report mark
    jump report

    .mark: increase
    iinc %1 static

    ; integer at 1 is *at least* N
    ; N is the parameter the function received
    if (not (lt %4 local %1 static (move %3 local %0 parameters) local) local) local finish

    .mark: report
    print %1 static
    frame ^[(copy %0 arguments %3 local)]
    tailcall counter/1

    .mark: finish
    return
.end

.function: main/1
    allocate_registers %2 local

    frame ^[(copy %0 arguments (integer %1 local 10) local)]
    call void counter/1
    izero %0 local
    return
.end
