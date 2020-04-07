.import: [[dynamic]] viuapq

.signature: viuapq::connect/1
.signature: viuapq::finish/1
.signature: viuapq::get/2

.function: main/0
    allocate_registers %3 local

    .name: 0 r0
    .name: iota connection
    .name: iota query

    text %connection local "dbname = pt_lab2"

    frame %1
    move %0 arguments %connection local
    call %connection local viuapq::connect/1

    text %query local "select * from temp"
    frame %2
    ptr %r0 local %connection local
    move %0 arguments %r0 local
    move %1 arguments %query local
    call %query local viuapq::get/2

    print %query local

    frame %1
    move %0 arguments %connection local
    call void viuapq::finish/1

    izero %0 local
    return
.end
