; Test support for float division.

.function: main/1
    fstore 1 3.14
    fstore 2 2.0
    fdiv 3 1 2
    print 3
    izero 0
    return
.end
