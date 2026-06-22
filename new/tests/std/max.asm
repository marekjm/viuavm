.section ".text"

; The function is called max_unsafe, because it is not completely reliable when
; called with mixed-type arguments.
; See the fn_min test case for more details.
.symbol max_unsafe
.label max_unsafe
    copy $0.l, $1.p
    lt $1.l, $0.p, $0.l
    if $1.l, max_unsafe_epilogue
    copy $0.l, $0.p
.label max_unsafe_epilogue
    return $0.l
