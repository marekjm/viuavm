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
    call 0 baz
    izero 0
    end
.end
