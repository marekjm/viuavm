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
.function: make_get_field_message/3
    allocate_registers %5 local

    .name: 0 message
    .name: iota tag
    .name: iota reply_to
    .name: iota q
    .name: iota field

    frame %1
    atom %tag local 'get_field'
    move %0 arguments %tag local
    call %message local make_tagged_message_impl/1

    atom %tag local 'reply_to'
    move %reply_to local %0 parameters
    structinsert %message local %tag local %reply_to local

    atom %tag local 'q'
    move %q local %1 parameters
    structinsert %message local %tag local %q local

    atom %tag local 'field'
    move %field local %2 parameters
    structinsert %message local %tag local %field local

    return
.end
.function: make_postgres_result_message/1
    allocate_registers %3 local

    .name: 0 message
    .name: iota tag
    .name: iota value

    frame %1
    atom %tag local 'postgres_result'
    move %0 arguments %tag local
    call %message local make_tagged_message_impl/1

    atom %tag local 'value'
    move %value local %0 parameters
    structinsert %message local %tag local %value local

    return
.end


.function: postgres_io_get_field_impl/2
    allocate_registers %6 local

    .name: 0 r0
    .name: iota pq_conn
    .name: iota message
    .name: iota reply_to
    .name: iota q
    .name: iota field

    move %pq_conn local %0 parameters
    move %message local %1 parameters
    print %pq_conn local
    print %message local

    atom %q local 'q'
    structremove %q local %message local %q local
    print %q local

    frame %2
    move %0 arguments %pq_conn local
    move %1 arguments %q local
    call %q local viuapq::get_one/2

    atom %field local 'field'
    structremove %field local %message local %field local
    ;structremove %field local %q local %field local
    print %q local
    print %field local

    frame %1
    move %0 arguments %field local
    call %field local make_postgres_result_message/1

    atom %reply_to local 'reply_to'
    structremove %reply_to local %message local %reply_to local

    send %reply_to local %field local

    return
.end
.function: postgres_io_actor/1
    allocate_registers %8 local

    .name: 0 r0
    .name: iota connection
    .name: iota message
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
    frame %2
    ptr %r0 local %connection local
    move %0 arguments %r0 local
    move %1 arguments %message local
    call void postgres_io_get_field_impl/2

    .mark: stage_shutdown
    ;frame %1
    ;move %0 arguments %connection local
    ;call void viuapq::finish/1

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

    frame %3
    self %r0 local
    move %0 arguments %r0 local
    string %r0 local "select count(*) from a"
    move %1 arguments %r0 local
    atom %r0 local 'count'
    move %2 arguments %r0 local
    call %r0 local make_get_field_message/3
    send %postgres_connection local %r0 local

    receive %r0 local infinity
    print %r0 local

    frame %0
    call %r0 local make_shutdown_message/0
    send %postgres_connection local %r0 local

    join void %postgres_connection local infinity

    izero %0 local
    return
.end
