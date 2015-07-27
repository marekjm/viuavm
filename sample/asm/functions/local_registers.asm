.function: set_69
    ress global
    istore 1 69
    end
.end

.function: main
    istore 1 42

    call (frame 0) set_69
    print 1

    ress global
    print 1

    izero 0
    end
.end
