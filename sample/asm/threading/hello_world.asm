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
    thread 3 print_lazy

    frame ^[(param 0 (strstore 2 "Hello multithreaded World! (2)"))]
    thread 4 print_eager

    ; this no-ops here are required for now
    ; main/1 must run longer than any child threads or
    ; the CPU will raise exception about main/1 orphaning threads
    nop
    nop
    nop

    izero 0
    end
.end
