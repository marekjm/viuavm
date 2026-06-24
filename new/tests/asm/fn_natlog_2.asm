.section ".text"

.symbol [[extern]] ln2
.symbol [[extern]] ln

.symbol [[entry_point]] main
.label main
    ; First, test the backend implementation of natural logarithm of 2.
    frame $0.a
    call $1.l, ln2

    ; Then, test the frontend implementation of natural logarithm of any X.
    ; This is necessary to see if the ln() function is dispatching its
    ; parameters properly.
    frame $2.a
    double $2.l, 2.0
    copy $0.a, $2.l
    li $1.a, 1
    call $3.l, ln

    ebreak
    return
