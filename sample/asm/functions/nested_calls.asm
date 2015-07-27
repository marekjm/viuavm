.function: foo
    print (istore 1 42)
    end
.end

.function: bar
    print (istore 1 69)
    call (frame 0) foo
    end
.end

.function: baz
    print (istore 1 1995)
    call (frame 0) bar
    end
.end

.function: bay
    print (istore 1 2015)
    call (frame 0) baz
    end
.end


.function: main
    call (frame 0) bay

    izero 0
    end
.end
