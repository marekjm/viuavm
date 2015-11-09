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
    end
.end

.function: main
    frame ^[(param 0 (strstore 1 "Hello multithreaded World! (1)"))]
    thread 2 print_lazy

    thjoin 2

    izero 0
    end
.end
