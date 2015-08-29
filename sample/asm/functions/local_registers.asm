.function: set_69
    ress global
    istore 2 69
    end
.end

.function: main
    istore 2 42

    call (frame 0) set_69
    print 2

    ress global
    print 2

    ress local
    izero 0
    end
.end
