.function: one
    print (istore 1 42)
    izero 0
    ; no end here
.end

.function: main
    call (frame 0 2) one
    izero 0
    end
.end
