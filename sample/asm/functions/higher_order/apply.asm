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

.function: square/1
    ; this function takes single integer as its argument,
    ; squares it and returns the result
    mul int64 0 (arg 1 0) 1
    return
.end

.function: apply/2
    ; this function applies another function on a single parameter
    ;
    ; this function is type agnostic
    ; it just passes the parameter without additional processing
    .name: 1 func
    .name: 2 parameter

    ; apply the function to the parameter...
    frame ^[(pamv 0 (arg parameter 1))]
    fcall 3 (arg func 0)

    ; ...and return the result
    move 0 3
    return
.end

.function: main/1
    ; applies function square/1(int) to 5 and
    ; prints the result
    istore 1 5
    function 2 square/1

    frame ^[(param 0 2) (param 1 1)]
    print (call 3 apply/2)

    izero 0
    return
.end
