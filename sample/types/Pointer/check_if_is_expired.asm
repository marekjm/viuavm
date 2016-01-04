.signature: Pointer::expired

.function: isExpired
    frame ^[(param 0 (arg 1 0))]
    call 2 Pointer::expired
    echo (strstore 3 "expired: ")
    move 0 (print 2)
    end
.end

.function: main
    istore 1 42
    ptr 2 1

    frame ^[(param 0 2)]
    call 0 isExpired

    delete 1

    frame ^[(param 0 2)]
    call 0 isExpired

    izero 0
    end
.end
