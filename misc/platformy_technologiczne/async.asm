;
;  Copyright (C) 2020 Marek Marecki
;
;  This file is part of Viua VM.
;
;  Viua VM is free software: you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation, either version 3 of the License, or
;  (at your option) any later version.
;
;  Viua VM is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
;


.import: [[dynamic]] viuapq

.signature: viuapq::connect/1
.signature: viuapq::finish/1
.signature: viuapq::get/2
.signature: viuapq::get_one/2


.function: make_tagged_message_impl/1
    allocate_registers %4 local

    .name: iota message
    .name: iota tag
    .name: iota key

    move %tag local %0 parameters

    struct %message local
    atom %key local 'tag'
    structinsert %message local %key local %tag local

    move %0 local %message local
    return
.end
.function: make_shutdown_message/0
    allocate_registers %2 local

    .name: iota tag
    atom %tag local 'shutdown'

    frame %1
    move %0 arguments %tag local
    call %0 local make_tagged_message_impl/1
    return
.end


.function: postgres_io_actor/1
    allocate_registers %9 local

    .name: 0 r0
    .name: iota connection
    .name: iota message
    .name: iota key
    .name: iota got_tag
    .name: iota tag_shutdown
    .name: iota tag_get_one_row
    .name: iota tag_get_all_rows
    .name: iota tag_get_field

    move %connection local %0 parameters

    receive %message local infinity

    atom %got_tag local 'tag'
    structremove %got_tag local %message local %got_tag local

    atom %tag_shutdown local 'shutdown'
    atom %tag_get_one_row local 'get_one_row'
    atom %tag_get_all_rows local 'get_all_rows'
    atom %tag_get_field local 'get_field'

    atomeq %tag_shutdown local %got_tag local %tag_shutdown local
    atomeq %tag_get_one_row local %got_tag local %tag_get_one_row local
    atomeq %tag_get_all_rows local %got_tag local %tag_get_all_rows local
    atomeq %tag_get_field local %got_tag local %tag_get_field local

    if %tag_shutdown local stage_shutdown +1
    if %tag_get_one_row local stage_get_one_row +1
    if %tag_get_all_rows local stage_get_all_rows +1
    if %tag_get_field local stage_get_field +1

    .mark: stage_weird
    jump the_end

    .mark: stage_get_one_row
    .mark: stage_get_all_rows
    .mark: stage_get_field

    .mark: stage_shutdown
    string %r0 local "Postgres I/O actor shutting down"
    print %r0 local
    return

    .mark: the_end
    frame %1
    move %0 arguments %connection local
    tailcall postgres_io_actor/1
.end


.function: main/0
    allocate_registers %2 local

    .name: 0 r0
    .name: iota postgres_connection

    ; begin POSTGRESQL CONNECTION
    text %postgres_connection local "dbname = pt_lab2"

    frame %1
    move %0 arguments %postgres_connection local
    call %postgres_connection local viuapq::connect/1

    frame %1
    move %0 arguments %postgres_connection local
    process %postgres_connection local postgres_io_actor/1
    ; end POSTGRESQL CONNECTION

    frame %0
    call %r0 local make_shutdown_message/0

    send %postgres_connection local %r0 local

    join void %postgres_connection local infinity

    izero %0 local
    return
.end
