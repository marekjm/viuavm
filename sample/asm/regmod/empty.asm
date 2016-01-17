.function: main
    istore 1 42

    ref 2 1
    empty 2

    ; check if register 2 is null and
    ; store the value in register 3
    isnull 3 2
    print 3

    izero 0
    return
.end
