.section ".text"

.symbol [[entry_point]] main
.label main
    ; Limit the arithmetic to 8 bits. Why? If the algoritmhs are correct they
    ; will work on any bit width, but using 8 bits means it is easier to write
    ; the tests as 255 is much easier to remember than whatever maximum 64-bit
    ; number is.
    li $1.l, 8u
    earithmeticwidth void, $1.l

    ;---------------------------------------------------------------------------
    ; MAIN BODY OF THE TEST
    ;----------------------


    ;-----------------------------------
    ; ZERO

    ; zero # zero
    li $1.l, 0u
    li $2.l, 0u
    add.saturate $3.l, $1.l, $2.l

    ; zero # one
    li $1.l, 0u
    li $2.l, 1u
    add.saturate $4.l, $1.l, $2.l

    ; zero # -one (same as max, but tests the -1
    li $1.l, 0u
    li $2.l, -1u
    add.saturate $5.l, $1.l, $2.l

    ; zero # max
    li $1.l, 0u
    li $2.l, 255u
    add.saturate $6.l, $1.l, $2.l


    ;-----------------------------------
    ; ONE

    ; one # zero
    li $1.l, 1u
    li $2.l, 0u
    add.saturate $7.l, $1.l, $2.l

    ; one # one
    li $1.l, 1u
    li $2.l, 1u
    add.saturate $8.l, $1.l, $2.l

    ; one # -one
    li $1.l, 1u
    li $2.l, -1u
    add.saturate $9.l, $1.l, $2.l

    ; one # max
    li $1.l, 1u
    li $2.l, 255u
    add.saturate $10.l, $1.l, $2.l


    ;-----------------------------------
    ; -ONE

    ; -one # zero
    li $1.l, -1u
    li $2.l, 0u
    add.saturate $11.l, $1.l, $2.l

    ; -one # one
    li $1.l, -1u
    li $2.l, 1u
    add.saturate $12.l, $1.l, $2.l

    ; -one # -one
    li $1.l, -1u
    li $2.l, -1u
    add.saturate $13.l, $1.l, $2.l

    ; -one # max
    li $1.l, -1u
    li $2.l, 255u
    add.saturate $14.l, $1.l, $2.l

    ebreak
    return
