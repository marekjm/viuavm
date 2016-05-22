.function: by_reference/0
    move 0 1
    return
.end

.function: main
    ; store a number in register
    istore 1 69

    ; create a closure and enclose a value by reference
    closure 2 by_reference/0
    enclose 2 1 1

    frame 0
    ; store return value in another register (it is a reference!)
    fcall 3 2

    ; assign different value to it
    istore 3 42

    ; check if return-by-reference is working (should print 42)
    print 1

    izero 0
    return
.end
