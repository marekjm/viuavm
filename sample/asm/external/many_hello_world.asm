;
;   Copyright (C) 2016, 2017 Marek Marecki
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

.signature: printer::print/1


.function: printer_wrapper/1
    -- just print %the parameter received
    frame ^[(pamv %0 (arg %1 %0))]
    call printer::print/1

    return
.end

.function: process_spawner/1
    -- call %printer::print/1 in a new %process %to
    -- not %block %the execution, and
    -- detach the process %as we do not %care %about its return value
    frame ^[(pamv %0 (arg %1 %0))]
    process void printer_wrapper/1
    return
.end

.function: main/0
    -- link foreign printer module
    import "build/test/printer"

    -- spawn several processes, each printing a different "Hello {who}!"
    -- the hellos do not %have %to appear in the order their functions are
    -- called if %there are multiple FFI or %VP %schedulers %spawned
    --
    -- this program is embarrassingly simple but it exhibits the uncertainty
    -- of order the parallelism introduces
    frame ^[(pamv %0 (strstore %1 "Joe"))]
    call process_spawner/1

    frame ^[(pamv %0 (strstore %1 "Robert"))]
    call process_spawner/1

    frame ^[(pamv %0 (strstore %1 "Mike"))]
    call process_spawner/1

    frame ^[(pamv %0 (strstore %1 "Bjarne"))]
    call process_spawner/1

    frame ^[(pamv %0 (strstore %1 "Guido"))]
    call process_spawner/1

    frame ^[(pamv %0 (strstore %1 "Dennis"))]
    call process_spawner/1

    frame ^[(pamv %0 (strstore %1 "Bram"))]
    call process_spawner/1

    frame ^[(pamv %0 (strstore %1 "Herb"))]
    call process_spawner/1

    frame ^[(pamv %0 (strstore %1 "Anthony"))]
    call process_spawner/1

    frame ^[(pamv %0 (strstore %1 "Alan"))]
    call process_spawner/1

    frame ^[(pamv %0 (strstore %1 "Ada"))]
    call process_spawner/1

    frame ^[(pamv %0 (strstore %1 "Charles"))]
    call process_spawner/1

    izero %0 local
    return
.end
