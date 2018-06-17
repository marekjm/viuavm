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

.function: print_number_of_params/
    print (argc %1 local) local
    return
.end

.function: main/1
    print (argc %1 local) local

    izero %2 local

    frame ^[(param %0 %2 local) (param %1 %2 local)]
    call void print_number_of_params/

    frame %0
    call void print_number_of_params/

    izero %0 local
    return
.end
