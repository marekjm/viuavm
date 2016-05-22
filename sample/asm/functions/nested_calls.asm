.function: foo/0
    print (istore 1 42)
    return
.end

.function: bar/0
    print (istore 1 69)
    call (frame 0) foo/0
    return
.end

.function: baz/0
    print (istore 1 1995)
    call (frame 0) bar/0
    return
.end

.function: bay/0
    print (istore 1 2015)
    call (frame 0) baz/0
    return
.end


.function: main
    call (frame 0) bay/0

    izero 0
    return
.end
