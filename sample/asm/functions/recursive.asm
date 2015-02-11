.def: rec__VOID__INT__INT 0
    .name: 1 counter
    .name: 2 zero

    ; unpack arguments
    arg 0 counter
    arg 1 zero

    ; decrease counter and check if it's less than zero
    idec counter
    ilt counter zero 3

    branch 3 :break_rec
    print counter
    frame 2
    param 0 counter
    paref 1 zero
    call rec__VOID__INT__INT

    .mark: break_rec
    end
.end

.def: main 0
    ; setup parameters
    istore 1 10
    istore 2 0

    ; create frame and pass parameters
    frame 2
    param 0 1
    paref 1 2
    call rec__VOID__INT__INT

    izero 0
    end
.end
