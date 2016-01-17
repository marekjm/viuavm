.function: foo
    return
; below should be .end and not .function
.function: bar

.function: main
    izero 0
    return
.end
