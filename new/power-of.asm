.section ".text"


; The factorial function is necessary since it is used as the denominator in
; exponentiation represented as a Taylor series.
; See https://en.wikipedia.org/wiki/Taylor_series#Exponential_function for more
; details.
.symbol factorial
.label factorial
    li $1.l, 2
    lt $2.l, $0.p, $1.l
    li $1.l, 1u
    if $2.l, factorial_epilogue

.label factorial_main
    copy $1.l, $0.p
    subi $2.l, $1.l, 1

.label factorial_loop
    mul $1.l, $1.l, $2.l
    subi $2.l, $2.l, 1
    if $2.l, factorial_loop

.label factorial_epilogue
    add $1.l, uzero, $1.l
    return $1.l


; See https://en.wikipedia.org/wiki/E_(mathematical_constant)
.symbol make_e_fl
.label make_e_fl
    ; This is the accumulator for the sum, and is the return value of the make_e
    ; function.
    float $1.l, 1.0

    ; This is the loop counter.
    li $2.l, 1u

    ; Just for float conversions.
    float $4.l, 1.0

.label make_e_fl_loop
    ; Obtain an item in the series: 1.0f / factorial(n)
    frame $1.a
    copy $0.a, $2.l
    call $3.l, factorial

    div $3.l, $4.l, $3.l

    ; Update the accumulator.
    add $1.l, $1.l, $3.l

    ; Increment the loop counter...
    addi $2.l, $2.l, 1u

    ; ...and check if the loop should be broken.
    gt $3.l, $2.l, $0.p
    not $3.l, $3.l
    if $3.l, make_e_fl_loop

    return $1.l


; See https://www.wolframalpha.com/input?i=taylor+ln%28x%29
.symbol ln
.label ln
    float $1.l, 1.0
    eq $1.l, $1.l, $0.p
    if $1.l, ln_error_of_1

    subi $1.l, $0.p, 1u
    li $2.l, 1u
    lt $2.l, $1.l, $2.l
    if $2.l, ln_lt_1
    if void, ln_gt_1

.label ln_error_of_1
    float $0.l, 0.0
    return $0.l
.label ln_lt_1
.label ln_gt_1
    halt


.symbol ln_base_impl
.label ln_base_impl
    li $1.l, -1  ; Constant -1 -- DO NOT TOUCH!

    li $2.l, 1u  ; k

    li $3.l, -1  ; accumulator for (-1)**k

    add $4.l, $1.l, $0.p  ; accumulator for (-1 + x)**k


; a**b = power-of(a, b) = power-of(e, (b * ln(a)))

.symbol [[entry_point]] main
.label main
    ; frame $1.a
    ; li $0.a, 9u
    ; call $2.l, make_e_fl
    ; ebreak

    frame $2.a
    li $0.a, -2
    li $1.a, 3u
    call $1.l, pown

    frame $2.a
    li $0.a, 2u
    li $1.a, 3u
    call $2.l, pown

    frame $2.a
    float $0.a, 1.41
    li $1.a, 3u
    call $3.l, pown

    frame $2.a
    double $0.a, 3.14159
    li $1.a, 3u
    call $4.l, pown

    frame $2.a
    double $0.a, 3.14159
    li $1.a, 0u
    call $4.l, pown

    frame $1.a
    li $0.a, -1
    call $5.l, abs

    frame $1.a
    li $0.a, 0u
    call $6.l, abs

    frame $1.a
    li $0.a, 1
    call $7.l, abs

    frame $1.a
    float $0.a, 1.41
    call $8.l, abs

    frame $1.a
    double $0.a, -3.14159
    call $9.l, abs

    ebreak

    return
