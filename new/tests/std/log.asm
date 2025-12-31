.section ".text"

;
; Logarithms
;


.symbol [[extern]] absr

; ln: x -> n -> r
;   where
;       x: any arithmetic type
;       n: number of iterations as a signed integer
;       r: real type
;
; Calculate the natural logarithm ie, a logarithm with the base of e.
.symbol ln
.label ln
    ; x'
    frame $1.a
    subi $0.a, $0.p, 1
    call $1.l, absr

    li $2.l, 1
    eq $3.l, $1.l, $2.l
    if $3.l, ln_of_2

    lt $3.l, $1.l, $2.l
    if $3.l, ln_of_lt1

.label ln_of_gt1
    frame $2.a
    copy $0.a, $0.p
    copy $1.a, $1.p
    call $0.l, ln_gt1
    return $0.l

.label ln_of_2
    frame $2.a
    copy $0.a, $0.p
    copy $1.a, $1.p
    call $0.l, ln2
    return $0.l

.label ln_of_lt1
    frame $2.a
    copy $0.a, $0.p
    copy $1.a, $1.p
    call $0.l, ln_lt1
    return $0.l


; ln2: r
;   where
;       r: real type
;
; Natural logarithm of 2, hardcoded.
.symbol ln2
.label ln2
    double $0.l, 0.693147180559945309417232121458
    return $0.l


; Needed for logarithms expressed as Taylor series.
.symbol [[extern]] pown
.symbol [[extern]] powz


; Same signature as ln.
.symbol ln_lt1
.label ln_lt1
    ; The accumulator
    double $1.l, 0.0

    ; The loop counter
    ; Also referred to as the variable k.
    li $2.l, 1

    ; The loop terminator
    addi $3.l, $1.p, 1

    ; The sign
    ;
    ; Even iterations ie, those where k is even, need a positive sign; wherease
    ; odd iterations need a negative sign.
    ; In the algorithm this is expressed as
    ;
    ;   -1 ** k
    ;
    ; but we can simplify this just flipping the value between -1 and 1 every
    ; iteration since we know that -1 to the power of X always results in either
    ; -1 or 1 anyway.
    ;
    ; Since we start with an odd-numbered iteration where k = 1, the sign should
    ; be initalised with -1.
    li $4.l, -1

    ; The x'
    ;
    ; The algorithm is:
    ;
    ;    ____ inf  ((-1) ** k) * ((-1 + x) ** k)
    ;  - \         -----------------------------
    ;    /___ k=1                k
    ;
    ;
    ; we can extract that (-1 + x) as x' = -1 + x, and we do not have to
    ; calculate x' every iteration.
    subi $5.l, $0.p, 1

.label ln_lt1_loop
    lt $6.l, $2.l, $3.l
    not $6.l, $6.l
    if $6.l, ln_lt1_epilogue

    frame $2.a
    copy $0.a, $5.l
    copy $1.a, $2.l
    call $6.l, pown

    mul $6.l, $4.l, $6.l
    div $6.l, $6.l, $2.l
    add $1.l, $1.l, $6.l

    ; Update sign
    muli $4.l, $4.l, -1

    ; Update k
    addi $2.l, $2.l, 1

    if void, ln_lt1_loop

.label ln_lt1_epilogue
    double $0.l, -1.0
    mul $0.l, $0.l, $1.l
    return $0.l


; Same signature as ln.
.symbol ln_gt1
.label ln_gt1
    ; The accumulator
    double $1.l, 0.0

    ; The loop counter
    ; Also referred to as the variable k.
    li $2.l, 1

    ; The loop terminator
    addi $3.l, $1.p, 1

    ; The sign
    ;
    ; Even iterations ie, those where k is even, need a positive sign; wherease
    ; odd iterations need a negative sign.
    ; In the algorithm this is expressed as
    ;
    ;   -1 ** k
    ;
    ; but we can simplify this just flipping the value between -1 and 1 every
    ; iteration since we know that -1 to the power of X always results in either
    ; -1 or 1 anyway.
    ;
    ; Since we start with an odd-numbered iteration where k = 1, the sign should
    ; be initalised with -1.
    li $4.l, -1

    ; The x'
    ;
    ; The algorithm is:
    ;
    ;    ____ inf  ((-1) ** k) * ((-1 + x) ** -k)
    ;    \         -----------------------------
    ;    /___ k=1                k
    ;
    ;
    ; we can extract that (-1 + x) as x' = -1 + x, and we do not have to
    ; calculate x' every iteration.
    subi $5.l, $0.p, 1

.label ln_gt1_loop
    lt $6.l, $2.l, $3.l
    not $6.l, $6.l
    if $6.l, ln_gt1_epilogue

    frame $2.a
    copy $0.a, $5.l
    muli $1.a, $2.l, -1
    call $7.l, powz

    ; sign * x'
    mul $8.l, $4.l, $7.l

    ; ... / k
    div $9.l, $8.l, $2.l

    ; Add the intermediate result to the accumulator.
    add $1.l, $1.l, $9.l

    ; Flip the sign
    muli $4.l, $4.l, -1

    ; Update k
    addi $2.l, $2.l, 1

    ebreak

    if void, ln_gt1_loop

.label ln_gt1_epilogue
    ; ln(x')
    frame $2.a
    copy $0.a, $5.l
    copy $1.a, $1.p
    call $10.l, ln

    ; And the final return value is
    ;
    ;   ln(x') - accumulator
    sub $0.l, $10.l, $1.l

    ebreak

    return $0.l
