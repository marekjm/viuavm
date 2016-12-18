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
    register (attach (class 1 Base) Base::saySomething/1 hello/1)
    register (attach (derive (class 1 Derived) Base) Derived::saySomethingMore/1 hello/1)

    return
.end

.function: Base::saySomething/1
    print (strstore 1 "Hello Base World!")
    return
.end

.function: Derived::saySomethingMore/1
    print (strstore 1 "Hello Derived World!")
    return
.end

.function: main/1
    frame 0
    call void typesystem_setup/0

    ; create a Base object and
    ; send a message to it
    frame ^[(param 0 (new 1 Base))]
    msg void hello/1

    ; create a Derived object and
    ; send a message to it
    frame ^[(param 0 (new 2 Derived))]
    msg void hello/1

    frame ^[(param 0 2)]
    call void Base::saySomething/1

    izero 0
    return
.end
