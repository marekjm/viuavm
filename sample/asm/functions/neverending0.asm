.function: one/0
    print (istore 1 42)
    izero 0
    ; no return here
.end

.function: main
    call (frame 0 2) one/0
    izero 0
    return
.end
