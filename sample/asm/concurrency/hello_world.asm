.function: print_lazy/1
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

.function: main
    frame ^[(param 0 (strstore 1 "Hello concurrent World! (1)"))]
    process 3 print_lazy/1

    frame ^[(param 0 (strstore 2 "Hello concurrent World! (2)"))]
    process 4 print_eager/1

    ; join processes
    join 0 3
    join 0 4

    izero 0
    return
.end
