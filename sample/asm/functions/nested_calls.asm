.function: foo
    print (istore 1 42)
    return
.end

.function: bar
    print (istore 1 69)
    call (frame 0) foo
    return
.end

.function: baz
    print (istore 1 1995)
    call (frame 0) bar
    return
.end

.function: bay
    print (istore 1 2015)
    call (frame 0) baz
    return
.end


.function: main
    call (frame 0) bay

    izero 0
    return
.end
