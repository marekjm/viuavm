.section ".text"


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


.symbol [[entry_point]] main
.label main
    frame $0.a
    call $1.l, ln2

    ebreak
    return void
