.section ".text"


.symbol [[extern]] powr


.symbol [[entry_point]] main
.label main
    ; This is relly shitty power function...
    frame $2.a
    double $0.a, 1.0
    double $1.a, 1.0
    call $1.l, powr

    frame $2.a
    double $0.a, 2.0
    double $1.a, 2.0
    call $2.l, powr

    frame $2.a
    double $0.a, 3.0
    double $1.a, 3.0
    call $3.l, powr

    frame $2.a
    double $0.a, 0.0
    double $1.a, 1.0
    call $4.l, powr

    frame $2.a
    double $0.a, 1.0
    double $1.a, 0.0
    call $5.l, powr

    frame $2.a
    double $0.a, 0.0
    double $1.a, 0.0
    call $6.l, powr

    frame $2.a
    double $0.a, -1.0
    double $1.a, 0.0
    call $7.l, powr

    frame $2.a
    double $0.a, 0.0
    double $1.a, -1.0
    call $8.l, powr

    frame $2.a
    double $0.a, 1.0
    double $1.a, -1.0
    call $9.l, powr

    frame $2.a
    double $0.a, 2.0
    double $1.a, -1.0
    call $10.l, powr

    frame $2.a
    double $0.a, 2.0
    double $1.a, -2.0
    call $11.l, powr

    ebreak
    return void
