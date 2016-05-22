.function: boolean/1
    move 0 (not (not (arg 1 0)))
    return
.end

.function: main 1
    izero 1
    istore 2 1

    frame ^[(param 0 1)]
    call 1 boolean/1

    frame ^[(param 0 2)]
    call 2 boolean/1

    and 3 1 2

    print 1
    print 2
    print 3

    izero 0
    return
.end
