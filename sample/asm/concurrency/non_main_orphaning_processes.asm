.function: print_lazy/1
    ; many nops to make the process run for a long time long
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    print (arg 1 0)
    return
.end

.function: print_eager/1
    print (arg 1 0)
    return
.end

.function: process_spawner/0
    frame ^[(param 0 (strstore 1 "Hello concurrent World! (1)"))]
    process 3 print_lazy/1

    frame ^[(param 0 (strstore 2 "Hello concurrent World! (2)"))]
    process 4 print_eager/1

    join 0 4
    ; do not join the process to test main/1 node orphaning detection
    ;join 0 3

    return
.end

.function: main
    frame 0
    call process_spawner/0

    izero 0
    return
.end
