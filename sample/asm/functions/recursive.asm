.function: rec__VOID__INT__INT
    .name: 1 counter
    .name: 2 zero

    ; unpack arguments
    arg counter 0
    arg zero 1

    ; decrease counter and check if it's less than zero
    branch (ilt 3 (idec counter) zero) break_rec
    print counter

    frame ^[(param 0 counter) (paref 1 zero)]
    call rec__VOID__INT__INT

    .mark: break_rec
    end
.end

.function: main
    ; create frame and set initial parameters
    frame ^[(param 0 (istore 1 10)) (paref 1 (istore 2 0))]
    call rec__VOID__INT__INT

    izero 0
    end
.end
