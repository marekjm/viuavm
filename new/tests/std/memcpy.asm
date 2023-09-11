; memcpy(3) implementation to be used by all tests.

.function: "std::memcpy"
    li $1, 0u

    g.lt $2.l, $1.l, $2.p
    g.not $2.l, $2.l
    if $2.l, 10

    ; load from src
    add $3.l, $1.p, $1.l
    lb $2, $3, 0

    ; store to dst
    add $3, $0.p, $1.l
    sb $2, $3, 0

    ; increase counter and repeat
    addi $1, $1, 1u
    jump 1

    move $2.l, $0.p
    return $2.l
.end
