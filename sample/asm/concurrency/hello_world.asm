.function: print_lazy
    nop
    nop
    nop
    nop
    nop
    print (arg 1 0)
    return
.end

.function: print_eager
    print (arg 1 0)
    return
.end

.function: main
    frame ^[(param 0 (strstore 1 "Hello multithreaded World! (1)"))]
    process 3 print_lazy

    frame ^[(param 0 (strstore 2 "Hello multithreaded World! (2)"))]
    process 4 print_eager

    ; join threads
    thjoin 0 3
    thjoin 0 4

    izero 0
    return
.end
