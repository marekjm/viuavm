.function: printer_function
    ; expects register 1 to be an enclosed object
    print 1
    return
.end

.function: main
    ; create a closure and enclose object in register 1 with it
    closure 2 printer_function
    enclose 2 1 (strstore 1 "Hello World!")

    ; call the closure (should print "Hello World!")
    frame 0
    fcall 0 2

    ; store 42 in register 1, keep in mind that register 1 holds a reference so
    ; the istore will rebind the reference - it will now point to Integer(42)
    istore 1 42

    ; call the closure (should print "42")
    frame 0
    fcall 0 2

    izero 0
    return
.end
