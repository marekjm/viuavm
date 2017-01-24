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

.function: counter/1
    ; FIXME static analyser does not handle swicthes between registers sets well
    ; switch to static register set
    ress static

    ; if register 1 is *not null* jump to increase marker
    ; otherwise continue execution to perform initial set up of static registers
    if (not (isnull %2 %1)) increase

    ; these instructions are executed only when 1 register was null
    ; they first setup static counter variable
    istore %1 0

    ; then, switch to local registers and...
    ress local
    ; 1) fake taking counter from static registers (it's zero during first pass anyway)
    istore %1 0
    ; 2) fetch the argument
    arg %3 %0
    ; 3) jump straight to report mark
    jump report

    .mark: increase
    ; FIXME static analyser does not handle swicthes between registers sets well
    iinc %1

    ; the copy is required because TMPRI moves objects instead
    ; of copying
    copy %4 %1
    move (tmpri %1) %4
    ress local
    tmpro %1

    ; integer at 1 is *at least* N
    ; N is the parameter the function received
    if (not (lt int64 %4 %1 (arg %3 %0))) finish

    .mark: report
    print %1
    frame ^[(param %0 %3)]
    tailcall counter/1

    .mark: finish
    return
.end

.function: main/1
    frame ^[(param %0 (istore %1 10))]
    call void counter/1
    izero %0 local
    return
.end
