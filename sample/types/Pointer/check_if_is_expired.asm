.signature: Pointer::expired/1

.function: isExpired/1
    frame ^[(param 0 (arg 1 0))]
    call 2 Pointer::expired/1
    echo (strstore 3 "expired: ")
    move 0 (print 2)
    return
.end

.function: main
    istore 1 42
    ptr 2 1

    frame ^[(param 0 2)]
    call 0 isExpired/1

    delete 1

    frame ^[(param 0 2)]
    call 0 isExpired/1

    izero 0
    return
.end
