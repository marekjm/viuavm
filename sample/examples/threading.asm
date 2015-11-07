.function: print_lazy
    nop
    nop
    nop
    nop
    nop
    ; uncomment nop below to segfault
    ;nop
    arg 1 0
    nop
    print 1
    end
.end

.function: print_eager
    nop
    ; uncomment nop below to segfault
    ;nop
    print (arg 1 0)
    end
.end

.function: main
    frame ^[(param 0 (strstore 1 "Hello concurrent World! (1)"))]
    thread print_lazy

    frame ^[(param 0 (strstore 2 "Hello concurrent World! (2)"))]
    thread print_eager

    izero 0
    end
.end
