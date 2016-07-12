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

; this program requires standard "typesystem" module to be available
.signature: typesystem::typeof/1


.function: typesystem_setup/0
    register (attach (class 1 Base) fn_base/1 good_day/1)
    register (attach (derive (class 1 Derived) Base) fn_derived/1 hello/1)
    register (attach (derive (class 1 MoreDerived) Derived) fn_more_derived/1 hi/1)

    return
.end

.function: fn_base/1
    echo (strstore 1 "Good day from ")
    frame ^[(param 0 (arg 2 0))]
    print (call 3 typesystem::typeof/1)
    return
.end

.function: fn_derived/1
    echo (strstore 1 "Hello from ")
    frame ^[(param 0 (arg 2 0))]
    print (call 3 typesystem::typeof/1)
    return
.end

.function: fn_more_derived/1
    echo (strstore 1 "Hi from ")
    frame ^[(param 0 (arg 2 0))]
    print (call 3 typesystem::typeof/1)
    return
.end

.function: main/1
    import "typesystem"

    ; setup the typesystem
    call (frame 0) typesystem_setup/0

    ; create a Derived object and
    ; call methods on it
    new 1 Derived

    ; Good day from Derived
    frame ^[(param 0 1)]
    msg 0 good_day/1

    ; hello from Derived
    frame ^[(param 0 1)]
    msg 0 hello/1

    ; print an empty line
    print (strstore 3 "")

    ; create a MoreDerived object and
    ; call methods on it
    new 2 MoreDerived

    ; Good day from MoreDerived
    frame ^[(param 0 2)]
    msg 0 good_day/1

    ; Hello from MoreDerived
    frame ^[(param 0 2)]
    msg 0 hello/1

    ; Hi from MoreDerived
    frame ^[(param 0 2)]
    msg 0 hi/1

    izero 0
    return
.end
