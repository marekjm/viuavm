.signature: std::vector::of/2

.function: return_integer/1
    arg 0 0
    return
.end

.function: main
    link std::vector

    register (class 1 Foo)

    frame ^[(pamv 0 (istore 1 8)) (pamv 1 (function 1 return_integer/1))]
    call 1 std::vector::of/2

    print 1

    izero 0
    return
.end
