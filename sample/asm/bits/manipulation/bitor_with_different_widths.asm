;
;   Copyright (C) 2017, 2018 Marek Marecki <marekjm@ozro.pw>
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

.function: main/0
    allocate_registers %5 local

    bits (.name: %iota longer) local 0b1101000100100111
    bits (.name: %iota shorter) local            0b1101

    print %longer local
    print %shorter local

    print (bitor %iota local %longer local %shorter local) local
    print (bitor %iota local %shorter local %longer local) local

    izero %0 local
    return
.end
