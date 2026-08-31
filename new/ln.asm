.section ".text"


.symbol [[extern]] absr


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


.symbol lngt2
.label lngt2
    double $1.l, 2.0
    return $1.l


.symbol lnlt2
.label lnlt2
    double $1.l, -2.0
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
