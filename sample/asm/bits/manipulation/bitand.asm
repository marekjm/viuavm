;
;   Copyright (C) 2017 Marek Marecki <marekjm@ozro.pw>
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
    not (izero (.name: %iota enabled) local)

    istore (.name: %iota bitwidth) local 8

    bits (.name: %iota first) local %bitwidth local
    bits (.name: %iota second) local %bitwidth local

    .name: %iota bit_index
    bitset %first local (istore %bit_index local 0) %enabled local
    bitset %first local (istore %bit_index local 2) %enabled local
    bitset %first local (istore %bit_index local 4) %enabled local
    bitset %first local (istore %bit_index local 5) %enabled local
    bitset %first local (istore %bit_index local 6) %enabled local
    bitset %first local (istore %bit_index local 7) %enabled local

    bitset %second local (istore %bit_index local 0) %enabled local
    bitset %second local (istore %bit_index local 3) %enabled local
    bitset %second local (istore %bit_index local 4) %enabled local
    bitset %second local (istore %bit_index local 5) %enabled local
    bitset %second local (istore %bit_index local 7) %enabled local

    print %first local
    print %second local

    bitand (.name: %iota third) local %first local %second local
    print %third local

    ;print (bits %2 local (istore %1 local 8) local) local
    ;print (bitnot %3 local %2 local)

    izero %0 local
    return
.end
