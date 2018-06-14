;
;   Copyright (C) 2017 Marek Marecki
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
    atom (.name: %iota an_atom) local 'an_atom'
    atom (.name: %iota an_atom_again) local 'an_atom'
    atom (.name: %iota another_atom) local 'another_atom'

    print (atomeq %iota %an_atom local %an_atom_again local) local
    print (atomeq %iota %an_atom local %another_atom local) local

    izero %0 local
    return
.end
