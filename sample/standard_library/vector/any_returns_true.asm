.function: is_not_negative
    arg 1 0
    igte 0 1 (izero 2)
    return
.end

.signature: std::vector::any/2

.function: main
    link std::vector

    vpush (vpush (vpush (vec 2) (istore 1 -1)) (istore 1 0)) (istore 1 1)

    frame ^[(param 0 2) (pamv 1 (function 4 is_not_negative))]
    call 5 std::vector::any/2
    print 5

    izero 0
    return
.end
