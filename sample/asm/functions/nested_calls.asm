.def: foo 0
    istore 1 42
    print 1
    end
.end

.def: bar 0
    istore 1 69
    print 1
    frame 0
    call foo
    end
.end

.def: baz 0
    istore 1 1995
    print 1
    frame 0
    call bar
    end
.end

.def: bay 0
    istore 1 2015
    print 1
    frame 0
    call baz
    end
.end


frame 0
call bay
halt
