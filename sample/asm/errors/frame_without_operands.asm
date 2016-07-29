.function: foo/0
    return
.end

.function: main/0
    frame
    call foo/0
    izero 0
    return
.end
