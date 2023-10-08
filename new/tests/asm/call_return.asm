.section ".text"

.symbol [[entry_point]] main
.label main
    frame $0.a
    call $1, dummy

    ebreak
    return void

.symbol [[local]] dummy
.label dummy
    atom $1, hello_world
    return $1
