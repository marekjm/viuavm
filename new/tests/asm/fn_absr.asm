.section ".text"

.symbol [[extern]] absr

.symbol [[entry_point]] main
.label main
    ; Test on a negative, zero, and positive signed integer.
    frame $1.a
    li $0.a, -1
    call $1.l, absr

    frame $1.a
    li $0.a, 0
    call $2.l, absr

    frame $1.a
    li $0.a, 1
    call $3.l, absr

    ; Test on a signed integer, a float, and a double. Not to test the
    ; algorithm, but just to see if the function will return the expected type.
    frame $1.a
    li $0.a, 1u
    call $4.l, absr

    frame $1.a
    float $0.a, -1.41
    call $5.l, absr

    frame $1.a
    double $0.a, -3.14159
    call $6.l, absr

    ebreak
    return
