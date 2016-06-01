.function: foo/3
    return
.end

.function: main/0
    frame 3
    param 3 (istore 1 1)
    call foo/3

    izero 0
    return
.end
