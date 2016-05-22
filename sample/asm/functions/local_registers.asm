.function: set_69/0
    ress global
    istore 2 69
    return
.end

.function: main
    istore 2 42

    call (frame 0) set_69/0
    print 2

    ress global
    print 2

    ress local
    izero 0
    return
.end
