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

.function: closure_printer/0
    ; it has to be 2, because 2 register has been bound
    print %2 local
    return
.end

.function: closure_setter/1
    arg %1 local %0

    ; overwrite bound value with whatever we got
    copy %2 local %1 local
    return
.end

.function: returns_closures/0
    ; create a vector to store closures
    vector %1 local %0 local %0

    ; create a value to be bound in both closures
    integer %2 local 42

    ; create two closures binding the same variable
    ; presto, we have two functions that are share some state
    closure %3 local closure_printer/0
    capture %3 local %2 %2 local

    closure %4 local closure_setter/1
    capture %4 local %2 %2 local

    ; push closures to vector...
    vpush %1 local %3 local
    vpush %1 local %4 local

    ; ...and return the vector
    ; vectors can be used to return multiple values as
    ; they can hold any Type-derived type
    move %0 local %1 local
    return
.end

.function: main/1
    frame %0
    call (.name: %iota the_closures) local returns_closures/0

    frame %0
    vat (.name: %iota printer_closure) local %the_closures local (izero %iota local) local
    call void *printer_closure local

    frame ^[(param %0 (integer %iota local 69) local)]
    vat (.name: %iota setter_closure) local %the_closures local (integer %iota local 1) local
    call void *setter_closure local

    frame %0
    call void *printer_closure local

    izero %0 local
    return
.end
