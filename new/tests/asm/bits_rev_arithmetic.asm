.section ".text"

.symbol [[entry_point]] main
.label main
    ;---------------------------------------------------------------------------
    ; Unsigned values
    li $1.l, 1u
    bitarev $1.l, $1.l

    li $2.l, 0xffffffff00000000u
    bitarev $2.l, $2.l

    li $3.l, 0x0000feedbeef0000u
    bitarev $3.l, $3.l

    li $4.l, 0x123456789abcdef0u
    bitarev $4.l, $4.l

    ;---------------------------------------------------------------------------
    ; Signed values
    li $5.l, 1
    bitarev $5.l, $5.l

    ;---------------------------------------------------------------------------
    ; Also check for signed values with limited width arithmetic.
    li $0.l, 8u
    earithmeticwidth void, $0.l
    li $6.l, 1
    bitarev $6.l, $6.l

    ebreak
    return
