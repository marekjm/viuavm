.section ".text"

.symbol [[entry_point]] main
.label main
    atom $1, ok
    atom $2, not_ok

    if void, here
    move $1, $2

.label here
    noop

    ebreak
    return
