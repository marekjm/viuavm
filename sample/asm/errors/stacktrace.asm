.function: foo
    print 1
    end
.end

.function: bar
    frame 0
    call foo
    end
.end

.function: baz
    istore 1 42
    frame 0
    call bar
    end
.end

.function: main
    frame 0
    call baz 0
    izero 0
    end
.end
