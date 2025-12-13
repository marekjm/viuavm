.section ".text"

.symbol sign
.label sign
    ; The zero of the appropriate type has to be calculated in all cases so we
    ; may as well do it first.
    mul $0.l, $0.p, zero
    mul $0.l, $0.l, $0.l  ; Turn a possible -0.0 into a 0.0

    ; Make a 1 of the appropriate type.
    gt $1.l, $0.p, zero
    add $1.l, $0.l, $1.l

    ; Make a -1 of the appropriate type.
    lt $2.l, $0.p, zero
    sub $2.l, $0.l, $2.l

    ; return = ($0.p > 0) ? 1 : 0
    moveif $0.l, $1.l, $0.l

    ; return = return ? return : -1
    moveif $0.l, $0.l, $2.l

    return $0.l


.symbol [[entry_point]] main
.label main
    ; signed integers
    frame $1.a
    li $0.a, -1
    call $1.l, sign

    frame $1.a
    li $0.a, 0
    call $2.l, sign

    frame $1.a
    li $0.a, 1
    call $3.l, sign

    ; unsigned integers
    frame $1.a
    li $0.a, 0
    call $4.l, sign

    frame $1.a
    li $0.a, 1
    call $5.l, sign

    ; floats
    frame $1.a
    float $0.a, -1.0
    call $6.l, sign

    frame $1.a
    float $0.a, 0.0
    call $7.l, sign

    frame $1.a
    float $0.a, 1.0
    call $8.l, sign

    ; There is no need to test for doubles or other arithmetic types, as the
    ; sign() function is type independent.

    ebreak
    return
