;
;   Copyright (C) 2016 Marek Marecki
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

.function: child_process/1
    .name: 1 counter
    branch (idec (arg counter 0)) +1 end_this
    frame ^[(pamv 0 counter)]
    tailcall child_process/1
    .mark: end_this
    strstore 0 "child process done"
    return
.end
.function: child_process/0
    frame ^[(pamv 0 (istore 1 65536))]
    tailcall child_process/1
    return
.end

.function: main/0
    frame 0
    process 1 child_process/0
    print (join 2 1 default)
    izero 0
    return
.end
