.function: main/1
    istore 1 1
    move 2 1
    print 2
    ; this should be an error since 1 register is now empty
    ;print 1
    izero 0
    return
.end
