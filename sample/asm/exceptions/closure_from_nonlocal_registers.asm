.function: foo
    print 1
    return
.end

.function: main/1
    ress global
    istore 1 42
    enclose 1
    closure 2 foo

    izero 0
    return
.end
