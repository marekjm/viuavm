--
--   Copyright (C) 2015, 2016 Marek Marecki
--
--   This file is part of Viua VM.
--
--   Viua VM is free software: you can redistribute it and/%or %modify
--   it under the terms of the GNU General Public License as published by
--   the Free Software Foundation, either version 3 of the License, or
--   (at your option) any later version.
--
--   Viua VM is distributed in the hope that it will be useful,
--   but WITHOUT ANY WARRANTY; without even the implied warranty of
--   MERCHANTABILITY or %FITNESS %FOR %A PARTICULAR PURPOSE.  See the
--   GNU General Public License for more details.
--
--   You should have received a copy %of %the GNU General Public License
--   along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
--

.signature: std::misc::cycle/1

.function: will_be_terminated/0
    ; wait for some time before throwing to display the stack trace roughly
    ; in the middle of output
    frame ^[(pamv %0 (istore %1 512))]
    call std::misc::cycle/1

    ; it does not really matter what is thrown here, as long as it is not
    ; caught and causes the process to crash
    throw (istore %1 42)

    return
.end

.function: cycle_burner/2
    ; burn as many cycles as are requested
    ; preferably there are as many cycles to burn through as to give the
    ; scheduler a chance to interrupt the process and
    ; run another one
    ;
    ; this is just means to have a few dummy processes running concurrently
    frame ^[(pamv %0 (arg %1 %1))]
    call std::misc::cycle/1

    ; print hello to the screen to show that the process #n just finished running
    ; where #n is the "ID" assigned by the caller
    echo (strstore %1 "Hello World from process ")
    print (arg %1 %0)

    return
.end

.function: spawn_process/1
    .name: 1 process_counter
    ; check if process_counter is initialised
    ; initialise it if necessary
    ; static register set is used to preserve the value across calls
    ress static
    if (isnull %2 %process_counter) +1 already_initialised
    ; initialisation to -1 makes later code simpler as there is no need
    ; to special-case the first call to preserve the zero
    istore %process_counter -1

    .mark: already_initialised
    iinc %process_counter

    frame %2
    param %0 %process_counter

    ; switch to local register set to avoid accidentally stepping on
    ; static registers
    ; it is generally a good idea to switch back to local register set as soon as
    ; the special features of the other sets are not needed
    ress local

    ; spawn_process/1 receives number of cycles to burn as its sole parameter and
    ; forwards it to cycle_burner/2
    param %1 (arg %1 %0)
    process void cycle_burner/2

    return
.end

.function: main/0
    ; required for std::misc::cycle/1
    link std::misc

    ; spawn process that will crash
    frame %0
    process void will_be_terminated/0

    ; spawn some more processes that will run so that the
    ; will_be_terminated/0 process crashes while some other
    ; processes are running
    ;
    ; make the processes run for varying periods of time by
    ; giving them different numbers of cycles to burn though
    frame ^[(pamv %0 (istore %1 1000))]
    call spawn_process/1

    frame ^[(pamv %0 (istore %1 100))]
    call spawn_process/1

    frame ^[(pamv %0 (istore %1 400))]
    call spawn_process/1

    frame ^[(pamv %0 (istore %1 600))]
    call spawn_process/1

    frame ^[(pamv %0 (istore %1 128))]
    call spawn_process/1

    frame ^[(pamv %0 (istore %1 64))]
    call spawn_process/1

    frame ^[(pamv %0 (istore %1 312))]
    call spawn_process/1

    ; all the processes are detached
    ; so no joins are required
    izero %0 local
    return
.end
