.function: foo
    ; register 1 is enclosed from 'returns_closure' function
    print 1
    return
.end

.function: returns_closure
    closure 2 foo
    move 0 (enclosecopy 2 1 (istore 1 42))
    return
.end

.function: main
    .name: 1 bar
    ; call function that returns the closure
    frame 0
    call bar returns_closure

    ; create frame for our closure and
    ; call it
    frame 0 0
    fcall 0 bar

    izero 0
    return
.end
