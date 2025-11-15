.section ".text"

.symbol fibonacci
.label fibonacci
    ; Handle fibonacci(0).
    not $1.l, $0.p
    if $1.l, fibonacci_zero

    ; Handle fibonacci(1) and fibonacci(2).
    li $1.l, 3u
    lt $1.l, $0.p, $1.l
    if $1.l, fibonacci_one_or_two

    ; We need to subtract 1 from the parameter because the first iteration is
    ; "done" by setting 1u and 1u as the starting numbers instead of 0u and 1u.
    subi $0.p, $0.p, 1u

    ; The working numbers ie, the two elements that we will be summing each
    ; iteration.
    li $1.l, 1u
    li $2.l, 1u

    ; Let's put the loop counter in 0.l so that everything above 2.l is
    ; considered free to use.
    li $0.l, 0u

.label fibonacci_loop
    ; Since we are always adding the 2nd working number to the 1st, the loop
    ; would be adding a constant number to the accumulator... unless we swap the
    ; working numbers every iteration!
    ;
    ; For the first iteration it does not matter, because both numbers are 1,
    ; but look at what happens in further iterations:
    ;
    ;    working numbers | after...
    ;     (1st) | (2nd)  |
    ;    -------+--------+----------
    ;         2 |    1   |  add
    ;         1 |    2   |  swap
    ;         3 |    2   |  add
    ;         2 |    3   |  swap
    ;         5 |    3   |  add
    ;         3 |    5   |  swap
    ;
    ; The swapping trick makes the smaller number always appear on the
    ; right-hand side of the addition ie, in the 1.l register. This is also
    ; really useful for locating the return value: it is always in 1.l without
    ; any further register shuffling.
    swap $1.l, $2.l

    ; If the counter is less than the number of iterations requested by the
    ; caller proceed to sum the working numbers.
    ; Otherwise, break the loop and return the value.
    lt $3.l, $0.l, $0.p
    not $3.l, $3.l
    if $3.l, fibonacci_epilogue

    add $1.l, $1.l, $2.l

    ; Increase the loop counter and go to the next iteration.
    addi $0.l, $0.l, 1u
    if void, fibonacci_loop

.label fibonacci_zero
    return uzero

.label fibonacci_one_or_two
    li $1.l, 1u

.label fibonacci_epilogue
    return $1.l

.symbol [[entry_point]] main
.label main
    frame $1.a
    li $0.a, 0u
    call $0.l, fibonacci

    frame $1.a
    li $0.a, 1u
    call $1.l, fibonacci

    frame $1.a
    li $0.a, 2u
    call $2.l, fibonacci

    frame $1.a
    li $0.a, 3u
    call $3.l, fibonacci

    frame $1.a
    li $0.a, 4u
    call $4.l, fibonacci

    frame $1.a
    li $0.a, 5u
    call $5.l, fibonacci

    frame $1.a
    li $0.a, 6u
    call $6.l, fibonacci

    frame $1.a
    li $0.a, 7u
    call $7.l, fibonacci

    frame $1.a
    li $0.a, 8u
    call $8.l, fibonacci

    frame $1.a
    li $0.a, 9u
    call $9.l, fibonacci

    frame $1.a
    li $0.a, 10u
    call $10.l, fibonacci

    frame $1.a
    li $0.a, 15u
    call $19.l, fibonacci

    frame $1.a
    li $0.a, 29u
    call $29.l, fibonacci

    frame $1.a
    li $0.a, 48u
    call $48.l, fibonacci

    ebreak

    return
