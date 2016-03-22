.signature: std::vector::of_ints/1

.function: main
    link std::vector

    frame ^[(pamv 0 (istore 1 8))]
    call 1 std::vector::of_ints/1

    print 1
    
    izero 0
    return
.end
