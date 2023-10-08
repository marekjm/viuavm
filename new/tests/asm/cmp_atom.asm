.section ".text"

.symbol [[entry_point]] main
.label main
    atom $1, hello_world
    atom $2, hello_world

    eq $3, $1, $2

    ebreak
    return
