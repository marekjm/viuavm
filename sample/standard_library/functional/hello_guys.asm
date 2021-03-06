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

.function: greetings/1
    allocate_registers %2 local

    echo (string %1 local "Hello ") local
    echo (move %1 local %0 parameters) local
    print (string %1 local '!') local
    return
.end

.signature: std::functional::apply/2

.function: main/1
    allocate_registers %3 local

    import std::functional

    function %1 local greetings/1

    frame ^[(copy %0 arguments %1 local) (copy %1 arguments (string %2 local "World") local)]
    call std::functional::apply/2

    frame ^[(copy %0 arguments %1 local) (copy %1 arguments (string %2 local "Joe") local)]
    call std::functional::apply/2

    frame ^[(copy %0 arguments %1 local) (copy %1 arguments (string %2 local "Mike") local)]
    call std::functional::apply/2

    izero %0 local
    return
.end
