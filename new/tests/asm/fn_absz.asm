.section ".text"

.symbol [[extern]] absz

.symbol [[entry_point]] main
.label main
    ; Test on a negative, zero, and positive signed integer.
    frame $1.a
    li $0.a, -1
    call $1.l, absz

    frame $1.a
    li $0.a, 0
    call $2.l, absz

    frame $1.a
    li $0.a, 1
    call $3.l, absz

    ; Test on an unsigned integer, a float, and a double. Not to test the
    ; algorithm, but just to see if the function will return the expected type.
    frame $1.a
    li $0.a, 1u
    call $4.l, absz

    frame $1.a
    float $0.a, -1.0
    call $5.l, absz

    frame $1.a
    double $0.a, -1.0
    call $6.l, absz

    ; This would break the function because the result would wrap and become
    ; negative again. A simple solution to this problem is to use mul.saturate
    ; instead of the native operation provided by the host platform.
    ; frame $1.a
    ; li $0.a, -9223372036854775808
    ; call $7.l, absz

    ebreak
    return
