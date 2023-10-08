.section ".text"

.symbol [[entry_point]] main
.label main
    atom $1, hello_world

    ebreak
    return void
