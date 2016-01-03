.signature: Pointer::expired

.function: main
    istore 1 42
    ptr 2 1

    frame ^[(param 0 2)]
    call 3 Pointer::expired
    echo (strstore 4 "expired: ")
    print 3

    delete 1

    frame ^[(param 0 2)]
    call 3 Pointer::expired
    echo 4
    print 3

    izero 0
    end
.end
