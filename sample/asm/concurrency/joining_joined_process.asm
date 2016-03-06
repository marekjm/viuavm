.function: print_lazy
    nop
    nop
    nop
    nop
    nop
    nop
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

.function: main
    frame ^[(param 0 (strstore 1 "Hello multithreaded World! (1)"))]
    thread 3 print_lazy

    ; this is OK
    thjoin 0 3

    ; this throws an exception
    thjoin 0 3

    izero 0
    return
.end
