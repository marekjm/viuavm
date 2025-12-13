.section ".text"

.symbol sign
.label sign
    ; The zero of the appropriate type has to be calculated in all cases so we
    ; may as well do it first.
    mul $0.l, $0.p, zero

    ; And if the parameter is zero we can just return the zero immediately.
    if $0.p, sign_nonzero
    return $0.l

.label sign_nonzero
    gt $1.l, $0.p, zero
    if $1.l, sign_positive

.label sign_negative
    subi $0.l, $0.l, 1
    if void, sign_epilogue

.label sign_positive
    addi $0.l, $0.l, 1

.label sign_epilogue
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
