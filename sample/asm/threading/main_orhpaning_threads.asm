.function: print_lazy
    nop
    nop
    nop
    nop
    nop
    print (arg 1 0)
    end
.end

.function: print_eager
    print (arg 1 0)
    end
.end

.function: main
    frame ^[(param 0 (strstore 1 "Hello multithreaded World! (1)"))]
    thread 1 print_lazy

    frame ^[(param 0 (strstore 2 "Hello multithreaded World! (2)"))]
    thread 2 print_eager

    izero 0
    end
.end
