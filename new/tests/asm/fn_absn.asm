.section ".text"

.symbol [[extern]] absn

.symbol [[entry_point]] main
.label main
    ; Test on a negative, zero, and positive signed integer.
    frame $1.a
    li $0.a, -1
    call $1.l, absn

    frame $1.a
    li $0.a, 0
    call $2.l, absn

    frame $1.a
    li $0.a, 1
    call $3.l, absn

    ; Test on a signed integer, a float, and a double. Not to test the
    ; algorithm, but just to see if the function will return the expected type.
    frame $1.a
    li $0.a, 1u
    call $4.l, absn

    frame $1.a
    float $0.a, -1.0
    call $5.l, absn

    frame $1.a
    double $0.a, -1.0
    call $6.l, absn

    ; While this does not work for absz(), it works perfectly fine for absn()
    ; for to a slightly larger positive capacity of unsigned integers.
    frame $1.a
    li $0.a, -9223372036854775808
    call $7.l, absn

    ebreak
    return
