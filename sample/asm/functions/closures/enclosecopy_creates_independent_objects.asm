.function: a_closure/0
    ; expects register 1 to be enclosed
    print 1
    return
.end

.function: main
    strstore 1 "Hello World!"

    closure 2 a_closure/0
    enclosecopy 2 1 1

    print 1
    ; call the closure
    frame 0
    fcall 0 2

    ; this should not affect the object enclosed a "a_closure"
    print (istore 1 42)

    ; call the closure
    frame 0
    fcall 0 2

    izero 0
    return
.end
