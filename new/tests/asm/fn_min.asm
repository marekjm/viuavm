.section ".text"

.symbol [[extern]] min_unsafe

.symbol [[entry_point]] main
.label main
    frame $2.a
    li $0.a, 0u
    li $1.a, 1u
    call $1.l, min_unsafe

    frame $2.a
    li $0.a, 0
    li $1.a, 1
    call $2.l, min_unsafe

    frame $2.a
    li $0.a, -1
    li $1.a, 1
    call $3.l, min_unsafe

    frame $2.a
    float $0.a, -1.41
    float $1.a, 1.41
    call $4.l, min_unsafe

    frame $2.a
    double $0.a, -3.14159
    double $1.a, 3.14159
    call $5.l, min_unsafe

    ; Mixed type works as long as the second argument ie, the left-hand side of
    ; the gt instruction, can be safely cast to the type of the first argument.
    ; For example, an unsigned zero can be losslessly cast to a signed zero and
    ; the code works correctly.
    frame $2.a
    li $0.a, -1
    li $1.a, 0u
    call $6.l, min_unsafe

    ; In presence of overflow the function will behave in a "weird" way.
    ; Since -1 cannot be losslessly cast to an unsigned value, the function will
    ; return the maximum unsigned value here, not the -1.
    ; Why?
    ; Because -1 on the right-hand side of gt will turn into exactly the same
    ; bit-pattern as the left-hand argument.
    ; It is the programmer's, or the compiler's, job to make sure the arguments
    ; to the function make sense.
    frame $2.a
    li $0.a, 0xffffffffffffffffu
    li $1.a, -1
    call $7.l, min_unsafe

    ebreak
    return
