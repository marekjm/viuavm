.function: boolean
    arg 0 1
    not 1
    not 1
    move 0 1
    end
.end

.function: main 1
    izero 1
    istore 2 1

    frame 1
    param 0 1
    call boolean 1

    frame 1
    param 0 2
    call boolean 2

    and 3 1 2

    print 1
    print 2
    print 3

    izero 0
    end
.end
