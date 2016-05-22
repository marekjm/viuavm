.function: boolean/1
    move 0 (not (not (arg 1 0)))
    return
.end

.function: main 1
    izero 1

    frame ^[(param 0 1)]
    call 1 boolean/1

    print (not (print 1))

    izero 0
    return
.end
