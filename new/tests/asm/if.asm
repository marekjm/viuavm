.section ".text"

.symbol [[entry_point]] main
.label main
    atom $1, equal
    atom $2, not_equal

    eq $3, $1, $2

    if $3, if_true

    move $3, $2
    if void, epilogue

.label if_true
    move $3, $1

.label epilogue
    ebreak
    return
