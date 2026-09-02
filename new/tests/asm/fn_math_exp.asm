.section ".text"


.symbol [[extern]] exp


.symbol [[entry_point]] main
.label main
    ; This is a really shitty exp function...
    frame $1.a
    double $0.a, 1.0
    call $1.l, exp

    frame $1.a
    double $0.a, 2.718281828459045
    call $2.l, exp

    frame $1.a
    copy $0.a, $1.l
    call $3.l, exp

    frame $1.a
    double $0.a, 42.0
    call $4.l, exp

    ebreak
    return void
