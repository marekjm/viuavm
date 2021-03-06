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

.function: square/1
    ; this function takes single integer as its argument,
    ; squares it and returns the result
    allocate_registers %2 local

    mul %0 local (move %1 local %0 parameters) %1 local
    return
.end

.function: apply/2
    ; this function applies another function on a single parameter
    ;
    ; this function is type agnostic
    ; it just passes the parameter without additional processing
    allocate_registers %4 local

    .name: 1 func
    .name: 2 parameter

    ; apply the function to the parameter...
    frame ^[(move %0 arguments (move %parameter local %1 parameters) local)]
    call %3 local (move %func local %0 parameters) local

    ; ...and return the result
    move %0 local %3 local
    return
.end

.function: main/1
    ; applies function square/1(int) to 5 and
    ; prints the result
    allocate_registers %4 local

    integer %1 local 5
    function %2 local square/1

    frame ^[(copy %0 arguments %2 local) (move %1 arguments %1 local)]
    print (call %3 local apply/2) local

    izero %0 local
    return
.end
