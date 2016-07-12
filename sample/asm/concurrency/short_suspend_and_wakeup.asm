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

.function: run_this_in_a_process/1
    print (strstore 1 "hi, I am process 0")
    return
.end

.function: process_waking_up_the_other_one/1
    print (strstore 1 "hi, I am process 1")

    print (strstore 6 "waking up process 0")
    frame ^[(param 0 (arg 2 0))]
    msg 0 wakeup/1

    return
.end

.function: main/1
    izero 1

    frame ^[(param 0 1)]
    process 2 run_this_in_a_process/1
    frame ^[(param 0 (ptr 3 2))]
    msg 0 detach/1

    print (strstore 6 "suspending process 0")
    frame ^[(param 0 3)]
    msg 0 suspend/1

    frame ^[(param 0 3)]
    process 4 process_waking_up_the_other_one/1

    join 0 4

    izero 0
    return
.end
