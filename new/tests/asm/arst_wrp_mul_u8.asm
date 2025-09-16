.section ".text"

.symbol [[entry_point]] main
.label main
    ; Limit the arithmetic to 8 bits. Why? If the algoritmhs are correct they
    ; will work on any bit width, but using 8 bits means it is easier to write
    ; the tests as -128 is much easier to remember than whatever most negative
    ; 64-bit number is.
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
    mul.wrap $3.l, $1.l, $2.l

    ; zero # one
    li $1.l, 0u
    li $2.l, 1u
    mul.wrap $4.l, $1.l, $2.l

    ; zero # -one
    li $1.l, 0u
    li $2.l, -1u
    mul.wrap $5.l, $1.l, $2.l


    ;-----------------------------------
    ; ONE

    ; one # zero
    li $1.l, 1u
    li $2.l, 0u
    mul.wrap $6.l, $1.l, $2.l

    ; one # one
    li $1.l, 1u
    li $2.l, 1u
    mul.wrap $7.l, $1.l, $2.l

    ; one # -one
    li $1.l, 1u
    li $2.l, -1u
    mul.wrap $8.l, $1.l, $2.l


    ;-----------------------------------
    ; -ONE

    ; -one # zero
    li $1.l, -1u
    li $2.l, 0
    mul.wrap $9.l, $1.l, $2.l

    ; -one # one
    li $1.l, -1u
    li $2.l, 1
    mul.wrap $10.l, $1.l, $2.l

    ; -one # -one
    li $1.l, -1u
    li $2.l, -1u
    mul.wrap $11.l, $1.l, $2.l

    ;-----------------------------------
    ; Special cases to test multiplication with additional, non-extreme values.

    ;---------------
    ; First case:
    ;
    ;  - 2
    ;  - a value 1 above half the representable positive range eg, 128 for 8
    ;    bits, or 32767 for 16 bits
    ;
    ; The "1 above half" value should trigger overflow in the positive case, but
    ; NOT trigger the overflow in the negative case. We exploit the "unevenness"
    ; of the twos-complement representation.
    li $1.l, 2u
    li $2.l, 128u
    mul.wrap $12.l, $1.l, $2.l

    ; Then, we reverse the order of operands ie, run X * 2 instead of 2 * X (as
    ; in the previous two cases).
    li $1.l, 128u
    li $2.l, 2u
    mul.wrap $13.l, $1.l, $2.l

    ebreak
    return
