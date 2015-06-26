.function: foo
    istore 1 42
    print 1
    end
.end

.function: bar
    istore 1 69
    print 1
    frame 0
    call foo
    end
.end

.function: baz
    istore 1 1995
    print 1
    frame 0
    call bar
    end
.end

.function: bay
    istore 1 2015
    print 1
    frame 0
    call baz
    end
.end


.function: main
    frame 0
    call bay
    izero 0
    end
.end
