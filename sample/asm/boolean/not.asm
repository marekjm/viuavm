.function: boolean
    arg 1 0
    not 1
    not 1
    move 0 1
    return
.end

.function: main 1
    izero 1

    frame 1
    param 0 1
    call 1 boolean

    print 1
    not 1
    print 1

    izero 0
    return
.end
