.function: foo/3
    return
.end

.function: main/0
    frame 3
    param 0 (istore 1 1)
    pamv 2 1
    call foo/3

    izero 0
    return
.end
