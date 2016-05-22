; purpose of this program is to compute n-th power of a number

.function: dummy/0
    ; this function is here to stress jump calculation
    izero 0
    return
.end

.function: power_of/2
    .name: 1 base
    .name: 2 exponent
    .name: 3 zero
    .name: 6 result

    ; store operands of the power-of operation
    arg base 0

    ; if the exponent is equal to zero, store 1 in first register and jump to print
    ; invert so short form of branch instruction can be used
    branch (not (ieq 4 (arg exponent 1) (izero zero))) algorithm
    istore result 1
    jump final

    ; now, we multiply in a loop
    .mark: algorithm
    .name: 5 counter
    istore counter 1
    ; in register 6, store the base of power as
    ; we will need it for multiplication
    istore result @base

    .mark: loop
    branch (ilt 4 counter exponent) 12 final
    imul result result base
    nop
    iinc counter
    jump loop

    ; final instructions
    .mark: final
    ; return result
    move 0 result
    return
.end

.function: main
    frame ^[(param 0 (istore 1 4)) (param 1 (istore 2 3))]
    print (call 1 power_of/2)

    izero 0
    return
.end
