.function: a_closure/0
    ; expects register 1 to be enclosed object
    print 1
    return
.end

.function: main
    closure 1 a_closure/0
    enclosemove 1 1 (strstore 2 "Hello World!")

    print (isnull 3 2)

    frame 0
    fcall 0 1

    izero 0
    return
.end
