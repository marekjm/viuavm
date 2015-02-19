.def: foo 0
    print 1
    end
.end

.def: bar 0
    frame 0
    call foo
    end
.end

.def: baz 0
    istore 1 42
    frame 0
    call bar
    end
.end

.def: main 1
    frame 0
    call baz 0
    izero 0
    end
.end
