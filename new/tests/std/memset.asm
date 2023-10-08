; memcmp(3) implementation to be used by all tests.

.section ".text"

.symbol "std::memset"
.label "std::memset"
.begin
    li $1, 0u

.label "std::memset::loop"
    eq $2.l, $1.l, $2.p
    if $2.l, "std::memset::epilogue"

    add $3.l, $0.p, $1.l
    sb $1.p, $3.l, 0
    addi $1.l, $1.l, 1u
    if void, "std::memset::loop"

.label "std::memset::epilogue"
    move $4.l, $0.p
    return $4.l
.end
