; memcmp(3) implementation to be used by all tests.

.section ".text"

.symbol "std::memcmp"
.label "std::memcmp"
; FIXME make .begin/.end automatically prefix nested labels
.begin
    li $1, 0u
    li $8.l, 0

.label "std::memcmp::loop"
    g.lt $2.l, $1.l, $2.p
    g.not $2.l, $2.l
    if $2.l, "std::memcmp::epilogue"

    ; load from s1
    add $3.l, $0.p, $1.l
    g.lb $4.l, $3.l, 0
    cast $4.l, uint

    ; load from s2
    add $5.l, $1.p, $1.l
    g.lb $6.l, $5.l, 0
    cast $6.l, uint

    g.cmp $8.l, $4.l, $6.l
    if $8.l, "std::memcmp::epilogue"

    ; increase counter and repeat
    addi $1, $1, 1u
    ; FIXME allow JUMP pseudoinstruction again
    if void, "std::memcmp::loop"

.label "std::memcmp::epilogue"
    return $8.l
.end
