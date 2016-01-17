.function: boolean
    arg 1 0
    not 1
    not 1
    move 0 1
    return
.end

.function: main 1
    izero 1
    istore 2 1

    frame 1
    param 0 1
    call 1 boolean

    frame 1
    param 0 2
    call 2 boolean

    and 3 1 2

    print 1
    print 2
    print 3

    izero 0
    return
.end
