.function: main
    istore 1 42
    ; empty the register
    free 1

    ; check if register 1 is null and
    ; store the value in register 2
    isnull 2 1
    print 2

    izero 0
    end
.end
