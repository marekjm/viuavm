.section ".text"

.symbol [[entry_point]] main
.label main
    atom $1.l, hello_world
    ebreak
    return
