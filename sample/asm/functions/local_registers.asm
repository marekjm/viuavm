.def: set_69 0
    ress global
    istore 1 69
    end
.end

.def: main 0
    istore 1 42
    frame 0
    call set_69
    print 1
    ress global
    print 1
    end
.end
