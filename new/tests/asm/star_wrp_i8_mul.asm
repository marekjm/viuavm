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
    li $1.l, 0
    li $2.l, 0
    mul.wrap $3.l, $1.l, $2.l

    ; zero # one
    li $1.l, 0
    li $2.l, 1
    mul.wrap $4.l, $1.l, $2.l

    ; zero # -one
    li $1.l, 0
    li $2.l, -1
    mul.wrap $5.l, $1.l, $2.l

    ; zero # max
    li $1.l, 0
    li $2.l, 127
    mul.wrap $6.l, $1.l, $2.l

    ; zero # min
    li $1.l, 0
    li $2.l, -128
    mul.wrap $7.l, $1.l, $2.l


    ;-----------------------------------
    ; ONE

    ; one # zero
    li $1.l, 1
    li $2.l, 0
    mul.wrap $8.l, $1.l, $2.l

    ; one # one
    li $1.l, 1
    li $2.l, 1
    mul.wrap $9.l, $1.l, $2.l

    ; one # -one
    li $1.l, 1
    li $2.l, -1
    mul.wrap $10.l, $1.l, $2.l

    ; one # max
    li $1.l, 1
    li $2.l, 127
    mul.wrap $11.l, $1.l, $2.l

    ; one # min
    li $1.l, 1
    li $2.l, -128
    mul.wrap $12.l, $1.l, $2.l


    ;-----------------------------------
    ; -ONE

    ; -one # zero
    li $1.l, -1
    li $2.l, 0
    mul.wrap $13.l, $1.l, $2.l

    ; -one # one
    li $1.l, -1
    li $2.l, 1
    mul.wrap $14.l, $1.l, $2.l

    ; -one # -one
    li $1.l, -1
    li $2.l, -1
    mul.wrap $15.l, $1.l, $2.l

    ; -one # max
    li $1.l, -1
    li $2.l, 127
    mul.wrap $16.l, $1.l, $2.l

    ; -one # min
    li $1.l, -1
    li $2.l, -128
    mul.wrap $17.l, $1.l, $2.l


    ;-----------------------------------
    ; MAX

    ; max # zero
    li $1.l, 127
    li $2.l, 0
    mul.wrap $18.l, $1.l, $2.l

    ; max # one
    li $1.l, 127
    li $2.l, 1
    mul.wrap $19.l, $1.l, $2.l

    ; max # -one
    li $1.l, 127
    li $2.l, -1
    mul.wrap $20.l, $1.l, $2.l

    ; max # max
    li $1.l, 127
    li $2.l, 127
    mul.wrap $21.l, $1.l, $2.l

    ; max # min
    li $1.l, 127
    li $2.l, -128
    mul.wrap $22.l, $1.l, $2.l


    ;-----------------------------------
    ; MIN

    ; min # zero
    li $1.l, -128
    li $2.l, 0
    mul.wrap $23.l, $1.l, $2.l

    ; min # one
    li $1.l, -128
    li $2.l, 1
    mul.wrap $24.l, $1.l, $2.l

    ; min # -one
    li $1.l, -128
    li $2.l, -1
    mul.wrap $25.l, $1.l, $2.l

    ; min # max
    li $1.l, -128
    li $2.l, 127
    mul.wrap $26.l, $1.l, $2.l

    ; min # min
    li $1.l, -128
    li $2.l, -128
    mul.wrap $27.l, $1.l, $2.l


    ;-----------------------------------
    ; Special cases to test multiplication with additional, non-extreme values.

    ;---------------
    ; First case:
    ;
    ;  - positive and negative 2
    ;  - a value 1 above half the representable positive range eg, 64 for 8
    ;    bits, or 32768 for 16 bits
    ;
    ; The "1 above half" value should trigger overflow in the positive case, but
    ; NOT trigger the overflow in the negative case. We exploit the "unevenness"
    ; of the twos-complement representation.
    li $1.l, 2
    li $2.l, 64
    mul.wrap $28.l, $1.l, $2.l

    li $1.l, -2
    li $2.l, 64
    mul.wrap $29.l, $1.l, $2.l

    ; Then, we reverse the order of operands ie, run X * 2 instead of 2 * X (as
    ; in the previous two cases).
    li $1.l, 64
    li $2.l, 2
    mul.wrap $30.l, $1.l, $2.l

    li $1.l, 64
    li $2.l, -2
    mul.wrap $31.l, $1.l, $2.l

    ;---------------
    ; Then, we take the two numbers we used before, but the "1 above half" value
    ; turns negative.
    li $1.l, 2
    li $2.l, -64
    mul.wrap $32.l, $1.l, $2.l

    li $1.l, -2
    li $2.l, -64
    mul.wrap $33.l, $1.l, $2.l

    ; Then, we reverse the order of operands ie, run X * 2 instead of 2 * X (as
    ; in the previous two cases).
    li $1.l, -64
    li $2.l, 2
    mul.wrap $34.l, $1.l, $2.l

    li $1.l, -64
    li $2.l, -2
    mul.wrap $35.l, $1.l, $2.l

    ;---------------
    ; Next case:
    ;
    ;  - a value just a little bit above half of representable range (to make it
    ;    easy, select the "1 above half" value and increase it by 1; but for the
    ;    8-bit case we may use the "funny number")
    ;  - positive and negative 2
    ;
    ; The "just above half" value should be big enough to trigger overflow in
    ; the positive and negative case. The logic is similar to the previous
    ; special case, but here we want to test overflow on both ends of the range.
    li $1.l, 2
    li $2.l, 69
    mul.wrap $36.l, $1.l, $2.l

    li $1.l, -2
    li $2.l, 69
    mul.wrap $37.l, $1.l, $2.l

    ; Reverse operand order.
    li $1.l, 69
    li $2.l, 2
    mul.wrap $38.l, $1.l, $2.l

    li $1.l, 69
    li $2.l, -2
    mul.wrap $39.l, $1.l, $2.l

    ; Negative "just above half".
    li $1.l, 2
    li $2.l, -69
    mul.wrap $40.l, $1.l, $2.l

    li $1.l, -2
    li $2.l, -69
    mul.wrap $41.l, $1.l, $2.l

    ; Negative "just above half", and reverse order of operands.
    li $1.l, -69
    li $2.l, 2
    mul.wrap $42.l, $1.l, $2.l

    li $1.l, -69
    li $2.l, -2
    mul.wrap $43.l, $1.l, $2.l

    ; The same logic as above could be used to test other combinations. For
    ; example:
    ;
    ;  - 4 and 1/4 of negative max
    ;  - 4 and (1/4 of negative max) + 1
    ;  - 7 and 1/7 of negative max
    ;  - 7 and (1/7 of negative max) + 1
    ;
    ; etc. You probably see where this is going.

    ;---------------
    ; And now for something completely different, let's see if a multiplication
    ; of non-extreme values producing a non-extreme result works. The general
    ; formula to see if two signed integers multiplied by each other will not
    ; overflow is:
    ;
    ;   - given an integer R on the right-hand side, and integer L on the
    ;     left-hand side
    ;   - take an integer L' that is the bit-reverse of L
    ;   - if L' < R, then overflow will not happen
    ;
    ; If R or L are negative, take their absolute value before applying the
    ; above algorithm.

    li $1.l, 8
    li $2.l, 8
    mul.wrap $44.l, $1.l, $2.l

    li $1.l, 8
    li $2.l, -8
    mul.wrap $45.l, $1.l, $2.l

    li $1.l, -8
    li $2.l, 8
    mul.wrap $46.l, $1.l, $2.l

    li $1.l, -8
    li $2.l, -8
    mul.wrap $47.l, $1.l, $2.l

    ebreak
    return
