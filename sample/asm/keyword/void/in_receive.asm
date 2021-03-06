;
;   Copyright (C) 2016, 2017, 2018 Marek Marecki
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

.function: sender/1
    allocate_registers %3 local
    send (move (.name: %iota pid) local %0 parameters) local (string %iota local "Hello World!") local
    return
.end

.function: main/0
    allocate_registers %2 local

    frame ^[(move %iota arguments (self %iota local) local)]
    process void sender/1

    receive void infinity

    izero %0 local
    return
.end
