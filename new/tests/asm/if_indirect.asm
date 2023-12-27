.section ".text"

.symbol [[entry_point]] main
.label main
    atom $1, equal
    atom $2, not_equal

    atxtp $4.l, @if_true

    eq $3, $1, $2

    if $3, $4.l

    move $3, $2
    if void, epilogue

.label if_true
    move $3, $1

.label epilogue
    ebreak
    return
