.def: foo
    print 1
    end
.end

.def: bar
    frame 0
    call foo
    end
.end

.def: baz
    istore 1 42
    frame 0
    call bar
    end
.end

.def: main
    frame 0
    call baz 0
    izero 0
    end
.end
