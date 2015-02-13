.def: fibonacci_memoizing 1
    izero 0
    end
.end

.def: main 1
    istore 1 1
    frame 1
    param 0 1
    call fibonacci_memoizing 2
    print 2

    izero 0
    end
.end
