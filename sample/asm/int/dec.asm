; Test support for idec instruction.

.function: main/1
    istore 1 2
    idec 1
    print 1
    izero 0
    return
.end
