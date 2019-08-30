.function: yep/2
    allocate_registers %6 local

    .name: 5 parent
    move %parent local %0 parameters

    .name: 1 fd_in
    .name: 3 stdout
    move %fd_in local %1 parameters
    integer %stdout local 1

    .name: 2 buffer_size
    integer %buffer_size local 8

    .name: 4 data
    io_read %data local %fd_in local %buffer_size local
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

    frame %2
    self %1 local
    move %0 arguments %1 local
    integer %1 local 0
    move %1 arguments %1 local
    process void yep/2

    receive %0 local 10s
    print %0 local
    io_wait %0 local %0 local 1s
    print %0 local

    izero %0 local
    return
.end
