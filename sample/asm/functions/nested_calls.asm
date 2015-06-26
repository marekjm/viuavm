.def: foo
    istore 1 42
    print 1
    end
.end

.def: bar
    istore 1 69
    print 1
    frame 0
    call foo
    end
.end

.def: baz
    istore 1 1995
    print 1
    frame 0
    call bar
    end
.end

.def: bay
    istore 1 2015
    print 1
    frame 0
    call baz
    end
.end


.def: main
    frame 0
    call bay
    izero 0
    end
.end
