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
    allocate_registers %4 local

    bits (.name: %iota bitstring) local 0b0

    if %bitstring local +1 after_first
    print (text %iota local "OH NOES") local
    .mark: after_first

    bits %bitstring 0x1
    if %bitstring +1 done
    print (text %iota local "OH YEAH") local

    .mark: done
    izero %0 local
    return
.end
