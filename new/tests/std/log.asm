.section ".text"


;
; Math - Logarithms
;


.symbol [[extern]] absr
.symbol [[extern]] pown
.symbol [[extern]] powz


; ln: R -> R
.symbol ln
.label ln
    ; special case for ln(1) = 0
    double $0.l, 1.0
    eq $0.l, $0.l, $0.p
    not $0.l, $0.l
    if $0.l, ln_of_ne1
    double $0.l, 0.0
    return $0.l

.label ln_of_ne1
    ; special case for ln(x <= 0) = error
    double $0.l, 0.0
    eq $1.l, $0.p, $0.l
    lt $2.l, $0.p, $0.l
    or $0.l, $1.l, $2.l
    not $0.l, $0.l
    if $0.l, ln_of_in_range

    double $0.l, 0.0
    div $0.l, $0.l, $0.l
    return $0.l

.label ln_of_in_range
    ; this is the |-1 + x|
    frame $1.a
    subi $0.a, $0.p, 1
    call $0.l, absr

    li $1.l, 1

    ; maybe |-1 + x| > 1?
    gt $2.l, $0.l, $1.l
    if $2.l, ln_of_gt2

    ; maybe |-1 + x| < 1?
    lt $2.l, $0.l, $1.l
    if $2.l, ln_of_lt2

    ; must be |-1 + x| = 1
    if void, ln_of_eq2

.label ln_of_gt2
    frame $1.a
    copy $0.a, $0.p
    call $1.l, lngt2
    return $1.l

.label ln_of_lt2
    frame $1.a
    copy $0.a, $0.p
    call $1.l, lnlt2
    return $1.l

; natural logarithm of 2, hardcoded.
.label ln_of_eq2
    ; HERE BE DRAGONS (IEEE 754 floating-point dragons)
    ;
    ; 64-bit floats in decimal representation have precision of 15 decimal
    ; places. DO NOT try to be clever and use more.
    double $0.l, 0.693147180559945
    return $0.l


; lnlt2: R -> R
;
;     inf
;   - SUM [(-1)^k * (-1 + x)^k] / k
;     k=1
;
.symbol lnlt2
.label lnlt2
    ; this is the accumulator
    double $0.l, 0.0

    ; this is the k
    li $1.l, 1

    ; this is the number of evaluations necessary to get a good enough result
    ;
    ; I arrived at 49 through experimentation:
    ;
    ;   - 15 decimal digits is the precision of IEEE 754
    ;   - 15 evaluations of the core formula do not yield a good enough result
    ;   - 30 evaluations ie, precision times 2, still do not yield a good enough
    ;     result
    ;   - 60 evaluations it, precision times 4, yield a good enough result
    ;   - 90 evaluations do not yield a better result than 60 evaluations
    ;   - 48 evaluations do not yield a worse result than 60 evaluations
    ;
    ; Therefore, we can stop at 48 evaluations, because at this point doing more
    ; work will not improve the output of the function.
    ; The limit is set to 49 because to make exponentiation work we start
    ; counting from 1, not 0.
    li $2.l, 49

.label lnlt2_loop_begin
    ; evalue the core formula...
    frame $2.a
    copy $0.a, $0.p  ; x
    copy $1.a, $1.l  ; k
    call $3.l, lnlt2_iter

    ; ...and update the accumulator with the result
    add $0.l, $0.l, $3.l

    ; increase the loop counter, k
    addi $1.l, $1.l, 1

    ; control the loop
    lt $4.l, $1.l, $2.l
    if $4.l, lnlt2_loop_begin

    ; this is the end result
    muli $0.l, $0.l, -1
    return $0.l

; lnlt2_iter: R -> N -> R
;
; A single iteration of the series ie, the following formula:
;
;   [(-1)^k * (-1 + x)^k] / k
;
.symbol lnlt2_iter
.label lnlt2_iter
    ; this is the x
    copy $1.l, $0.p

    ; this is the k
    copy $2.l, $1.p

    ; this is the (-1)^k
    frame $2.a
    double $0.a, -1.0
    copy $1.a, $2.l
    call $3.l, pown

    ; this is the (-1 + x)^k
    frame $2.a
    addi $0.a, $1.l, -1
    copy $1.a, $2.l
    call $4.l, pown

    ; this is the numerator
    mul $5.l, $3.l, $4.l

    ; this is the end result
    div $0.l, $5.l, $2.l
    return $0.l


; lngt2: R -> R
;
;                  inf
;   ln(-1 + x) - { SUM [(-1)^k * (-1 + x)^k] / k }
;                  k=1
;
.symbol lngt2
.label lngt2
    ; this is the ln(-1 + x)
    frame $1.a
    addi $0.a, $0.p, -1
    call $1.l, ln

    ; this is the accumulator
    double $0.l, 0.0

    ; this is the k
    li $2.l, 1

    ; this is the number of evaluations necessary to get a good enough result
    ; See the comment in lnlt2 to learn why this specific magic number is used.
    li $3.l, 49

.label lngt2_loop_begin
    ; evaluate the core formula...
    frame $2.a
    copy $0.a, $0.p  ; x
    copy $1.a, $2.l  ; k
    call $4.l, lngt2_iter

    ; ...and update the accumulator with the result
    add $0.l, $0.l, $4.l

    ; increase the loop counter, k
    addi $2.l, $2.l, 1

    ; control the loop
    lt $4.l, $2.l, $3.l
    if $4.l, lngt2_loop_begin

    ; this is the end result
    ; combine the two ingredients of the overall formula
    ; this is the ln(-1 + x) part minus the SUM part
    sub $0.l, $1.l, $0.l
    return $0.l


; lngt2_iter: R -> N -> R
;
; A single iteration of the series ie, the following formula:
;
;   [(-1)^k * (-1 + x)^(-k)] / k
;
.symbol lngt2_iter
.label lngt2_iter
    ; this is the x
    copy $1.l, $0.p

    ; this is the k
    copy $2.l, $1.p

    ; this is the -k
    muli $3.l, $2.l, -1

    ; this is the (-1)^k
    frame $2.a
    double $0.a, -1.0
    copy $1.a, $2.l
    call $4.l, pown

    ; this is the (-1 + x)^(-k)
    frame $2.a
    addi $0.a, $1.l, -1
    copy $1.a, $3.l
    call $5.l, powz

    ; this is the numerator
    mul $6.l, $4.l, $5.l

    ; this is the end result
    div $0.l, $6.l, $2.l
    return $0.l
