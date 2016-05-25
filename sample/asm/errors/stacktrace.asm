.function: foo
    print 1
    return
.end

.function: bar
    frame 0
    call foo
    return
.end

.function: baz
    istore 1 42
    frame 0
    call bar
    return
.end

.function: main/1
    frame 0
    call 0 baz
    izero 0
    return
.end
