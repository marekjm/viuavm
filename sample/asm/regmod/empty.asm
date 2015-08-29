.function: main
    istore 1 42

    ref 2 1
    ; empty the register holding a reference to the object
    ; this way memory leak is avoided
    ; should register 1. has been emptied, the memory would be leaked since
    ; the only non-reference pointer to the object would be lost
    empty 2

    ; check if register 2 is null and
    ; store the value in register 3
    isnull 3 2
    print 3

    izero 0
    end
.end
