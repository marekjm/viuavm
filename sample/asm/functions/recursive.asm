.function: recursive/2
    .name: 1 counter
    .name: 2 zero

    ; unpack arguments
    arg counter 0
    arg zero 1

    ; decrease counter and check if it's less than zero
    branch (ilt 3 (idec counter) zero) break_rec
    print counter

    frame ^[(param 0 counter) (pamv 1 zero)]
    call recursive/2

    .mark: break_rec
    return
.end

.function: main
    ; create frame and set initial parameters
    frame ^[(param 0 (istore 1 10)) (pamv 1 (istore 2 0))]
    call recursive/2

    izero 0
    return
.end
