.function: is_not_negative/1
    igte 0 (arg 1 0) (izero 2)
    return
.end

.signature: std::vector::any/2

.function: main
    link std::vector

    vpush (vpush (vpush (vec 2) (istore 1 -1)) (istore 1 -2)) (istore 1 -3)

    frame ^[(param 0 2) (pamv 1 (function 4 is_not_negative/1))]
    call 5 std::vector::any/2
    print 5

    izero 0
    return
.end
