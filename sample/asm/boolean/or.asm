.function: boolean/1
    move 0 (not (not (arg 1 0)))
    return
.end

.function: main 1
    frame ^[(pamv 0 (izero 1))]
    call 1 boolean/1

    frame ^[(pamv 0 (istore 2 1))]
    call 2 boolean/1

    or 3 1 2

    print 1
    print 2
    print 3

    izero 0
    return
.end
