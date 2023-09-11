; memcmp(3) implementation to be used by all tests.

.function: "std::memcmp"
    li $1, 0u
    li $8.l, 0

    g.lt $2.l, $1.l, $2.p
    g.not $2.l, $2.l
    if $2.l, 15

    ; load from s1
    add $3.l, $0.p, $1.l
    g.lb $4.l, $3.l, 0
    cast $4.l, uint

    ; load from s2
    add $5.l, $1.p, $1.l
    g.lb $6.l, $5.l, 0
    cast $6.l, uint

    g.cmp $8.l, $4.l, $6.l
    if $8.l, 15

    ; increase counter and repeat
    addi $1, $1, 1u
    jump 2

    return $8.l
.end
