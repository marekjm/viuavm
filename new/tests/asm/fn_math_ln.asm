.section ".text"


.symbol [[extern]] ln


.symbol [[entry_point]] main
.label main
    frame $1.a
    double $0.a, 2.0
    call $1.l, ln

    frame $1.a
    double $0.a, 0.5
    call $2.l, ln

    frame $1.a
    double $0.a, 3.0
    call $3.l, ln

    frame $1.a
    double $0.a, 1.0
    call $4.l, ln

    frame $1.a
    double $0.a, 2.718  ; roughly e
    call $5.l, ln

    frame $1.a
    double $0.a, 0.0  ; undefined = error
    call $6.l, ln

    frame $1.a
    double $0.a, -1.0  ; out of domain = error
    call $7.l, ln
    

    ebreak
    return void
