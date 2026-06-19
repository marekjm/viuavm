.section ".text"

; The function is called min_unsafe, because it is not completely reliable when
; called with mixed-type arguments.
; See the fn_min test case for more details.
.symbol min_unsafe
.label min_unsafe
    copy $0.l, $1.p
    gt $1.l, $0.p, $0.l
    if $1.l, min_unsafe_epilogue
    copy $0.l, $0.p
.label min_unsafe_epilogue
    return $0.l
