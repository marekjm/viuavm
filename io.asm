.function: yep/0
    allocate_registers %5 local

    integer %1 local 0
    integer %3 local 1

    integer %2 local 8
    io_read %4 local %1 local %2 local
    print %4 local
    ;io_cancel %4 local
    io_wait %4 local %4 local 1s
    print %4 local
    ;io_write %4 local %3 local %4 local

    ;integer %2 local 8
    ;io_read %4 local %1 local %2 local
    ;io_write %4 local %3 local %4 local

    ;integer %2 local 8
    ;io_read %4 local %1 local %2 local
    ;io_write %4 local %3 local %4 local

    return
.end

.function: main/0
    allocate_registers %1 local

    frame %0
    process void yep/0

    receive %0 local 5s
    print %0 local

    izero %0 local
    return
.end
