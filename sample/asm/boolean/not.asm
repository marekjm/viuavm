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

.function: boolean/1
    allocate_registers %2 local

    move %0 local (not (not (move %1 local %0 parameters local) local) local) local
    return
.end

.function: main/1
    allocate_registers %2 local

    izero %1 local

    frame ^[(copy %0 arguments %1 local)]
    call %1 local boolean/1

    print (not (print %1 local) local) local

    izero %0 local
    return
.end
