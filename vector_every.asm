.function: is_even
    .name: 2 two
    istore 2 2
    arg 1 0
    copy 4 1

    isub 1 1 2
    branch (ilt 3 1 2) +1 -2

    izero 0
    ieq 0 0 1

    echo 4
    echo (strstore 4 " ")
    print 0

    return
.end

.function: is_not_negative
    arg 1 0
    igte 0 1 (izero 2)
    return
.end

.function: debug_dafuq
    return
.end

.signature: std::vector::every/2
.signature: std::vector::of_ints/1

.function: main
    link std::vector

    frame ^[(param 0 (istore 1 20))]
    call 2 std::vector::of_ints/1
    print 2

    frame ^[(param 0 2) (pamv 1 (function 4 is_not_negative))]
    call 5 std::vector::every/2
    echo (strstore 4 "every? ")
    print 5

    izero 0
    return
.end
