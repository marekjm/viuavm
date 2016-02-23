.function: variable_changing_function
    ; expects register 1 to be an enclosed object
    istore 1 42
    return
.end

.function: main
    ; create a closure and enclose object in register 1 with it
    closure 2 variable_changing_function
    enclose 2 1 (strstore 1 "Hello World!")

    ; should print "Hello World!"
    print 1

    ; call the closure
    frame 0
    fcall 0 2

    ; should print "42"
    print 1

    izero 0
    return
.end
