.function: adder/1
    iadd 0 (arg 0 0) (istore 1 21)
    return
.end

.signature: std::functional::apply/2

.function: main
    link std::functional

    frame ^[(pamv 0 (function 1 adder/1)) (pamv 1 (istore 1 21))]
    call 1 std::functional::apply/2
    print 1

    izero 0
    return
.end
