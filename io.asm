.function: yep/1
    allocate_registers %6 local

    .name: 5 parent
    move %parent local %0 parameters

    .name: 1 stdin
    .name: 3 stdout
    integer %stdin local 0
    integer %stdout local 1

    .name: 2 buffer_size
    integer %buffer_size local 8

    .name: 4 data
    io_read %data local %stdin local %buffer_size local
    print %data local

    ;io_cancel %4 local
    io_wait %data local %data local 10s

    ;print %4 local
    io_write %data local %stdout local %data local
    ;io_wait %data local %data local 1s

    send %parent local %data local

    return
.end

.function: main/0
    allocate_registers %2 local

    self %1 local
    frame %1
    move %0 arguments %1 local
    process void yep/1

    receive %0 local 10s
    print %0 local
    io_wait %0 local %0 local 1s
    print %0 local

    izero %0 local
    return
.end
