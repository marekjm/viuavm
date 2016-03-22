.signature: std::vector::of_ints/1
.signature: std::vector::reverse/1

.function: main
    link std::vector

    frame ^[(pamv 0 (istore 1 8))]
    call 1 std::vector::of_ints/1
    print 1

    frame ^[(param 0 1)]
    call 2 std::vector::reverse/1
    print 2

    izero 0
    return
.end
