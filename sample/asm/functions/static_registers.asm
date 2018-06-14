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

.function: counter/1
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
    arg %3 local %0
    ; 3) jump straight to report mark
    jump report

    .mark: increase
    iinc %1 static

    ; integer at 1 is *at least* N
    ; N is the parameter the function received
    if (not (lt %4 local %1 static (arg %3 local %0) local) local) local finish

    .mark: report
    print %1 static
    frame ^[(param %0 %3 local)]
    tailcall counter/1

    .mark: finish
    return
.end

.function: main/1
    frame ^[(param %0 (integer %1 local 10) local)]
    call void counter/1
    izero %0 local
    return
.end
