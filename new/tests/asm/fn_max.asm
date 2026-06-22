.section ".text"

.symbol [[extern]] max_unsafe

.symbol [[entry_point]] main
.label main
    frame $2.a
    li $0.a, 0u
    li $1.a, 1u
    call $1.l, max_unsafe

    frame $2.a
    li $0.a, 0
    li $1.a, 1
    call $2.l, max_unsafe

    frame $2.a
    li $0.a, -1
    li $1.a, 1
    call $3.l, max_unsafe

    frame $2.a
    float $0.a, -1.41
    float $1.a, 1.41
    call $4.l, max_unsafe

    frame $2.a
    double $0.a, -3.14159
    double $1.a, 3.14159
    call $5.l, max_unsafe

    ; Mixed type works as long as the second argument ie, the left-hand side of
    ; the gt instruction, can be safely cast to the type of the first argument.
    ; For example, an unsigned zero can be losslessly cast to a signed zero and
    ; the code works correctly.
    frame $2.a
    li $0.a, -1
    li $1.a, 0u
    call $6.l, max_unsafe

    ; max_unsafe will accidentally do the right thing here, because -1 will turn
    ; into maxiumum unsigned value.
    frame $2.a
    li $0.a, 0xffffffffffffffffu
    li $1.a, -1
    call $7.l, max_unsafe

    ebreak
    return
