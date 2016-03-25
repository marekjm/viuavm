.function: adder
    iadd 0 (arg 0 0) (istore 1 21)
    return
.end

.signature: std::functional::apply

.function: main
    link std::functional

    frame ^[(pamv 0 (function 1 adder)) (pamv 1 (istore 1 21))]
    call 1 std::functional::apply
    print 1

    izero 0
    return
.end
