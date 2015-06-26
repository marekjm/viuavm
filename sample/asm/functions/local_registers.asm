.def: set_69
    ress global
    istore 1 69
    end
.end

.def: main
    istore 1 42
    frame 0
    call set_69
    print 1
    ress global
    print 1
    izero 0
    end
.end
