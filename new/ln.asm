.section ".text"


.symbol [[extern]] absr
.symbol [[extern]] pown


; ln2: f64
;
; Natural logarithm of 2, hardcoded.
.symbol ln2
.label ln2
    ; HERE BE DRAGONS (IEEE 754 floating-point dragons)
    ;
    ; 64-bit floats in decimal representation have precision of 15 decimal
    ; places. DO NOT try to be clever and use more.
    double $0.l, 0.693147180559945
    return $0.l


; ln: t -> t
.symbol ln
.label ln
    frame $1.a
    subi $0.a, $0.p, 1
    call $0.l, absr

    li $1.l, 1

    gt $2.l, $0.l, $1.l
    if $2.l, ln_of_gt2

    lt $2.l, $0.l, $1.l
    if $2.l, ln_of_lt2

    frame $0.a
    call $1.l, ln2
    return $1.l

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


; lnlt2: R -> R
.symbol lnlt2
.label lnlt2
    ; this is the accumulator
    double $0.l, 0.0

    ; this is the k
    li $1.l, 1

    frame $2.a
    copy $0.a, $0.p  ; x
    copy $1.a, $1.l  ; k
    call $2.l, lnlt2_iter

    add $0.l, $0.l, $2.l
    addi $1.l, $1.l, 1

    ; ebreak

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
    ; This is the x.
    copy $1.l, $0.p

    ; This is the k.
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
    ebreak
    return $0.l


.symbol lngt2
.label lngt2
    double $1.l, 2.0
    return $1.l


.symbol [[entry_point]] main
.label main
    frame $0.a
    call $1.l, ln2

    frame $1.a
    double $0.a, 2.0
    call $2.l, ln

    frame $1.a
    double $0.a, 3.0
    call $3.l, ln

    frame $1.a
    double $0.a, 0.5
    call $4.l, ln

    ebreak
    return void
