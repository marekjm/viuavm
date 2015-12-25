.function: print_lazy
    ; many nops to make the thread run for a long time long
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

.function: print_eager
    print (arg 1 0)
    end
.end

.function: main
    frame ^[(param 0 (strstore 1 "Hello multithreaded World! (1)"))]
    thread 3 print_lazy

    frame ^[(param 0 (strstore 2 "Hello multithreaded World! (2)"))]
    thread 4 print_eager

    thjoin 4
    ; do not join the thread to test main/1 node orphaning detection
    ;thjoin 3

    izero 0
    end
.end
