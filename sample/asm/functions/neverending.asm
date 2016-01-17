.function: one
    print (istore 1 42)
    izero 0
    ; no return here
.end

.function: two
    print (istore 1 48)
    izero 0
    return
.end

.function: main
    call (frame 0 2) one
    izero 0
    return
.end
