.function: print_lazy/1
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

.function: main/1
    frame ^[(param 0 (strstore 1 "Hello concurrent World! (1)"))]
    process 3 print_lazy/1

    ; this is OK
    join 0 3

    ; this throws an exception
    join 0 3

    izero 0
    return
.end
