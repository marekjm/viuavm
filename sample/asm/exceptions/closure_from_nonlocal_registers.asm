.def: foo 0
    print 1
    end
.end

.def: main 1
    ress global
    istore 1 42
    clbind 1
    closure foo 2

    izero 0
    end
.end
