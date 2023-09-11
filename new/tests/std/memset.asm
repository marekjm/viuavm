; memcmp(3) implementation to be used by all tests.

.function: "std::memset"
    li $1, 0u

    eq $2.l, $1.l, $2.p
    if $2.l, 7

    add $3.l, $0.p, $1.l
    sb $1.p, $3.l, 0
    addi $1.l, $1.l, 1u
    jump 1

    move $4.l, $0.p
    return $4.l
.end
