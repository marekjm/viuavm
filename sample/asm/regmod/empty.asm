.function: main
    istore 1 42
    ; empty the register
    empty 1

    ; check if register 1 is null and
    ; store the value in register 2
    isnull 1 2
    print 2

    izero 0
    end
.end
