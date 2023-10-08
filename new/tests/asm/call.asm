.section ".rodata"

.label hello_world
.object string "hello_world"

.section ".text"

.symbol [[entry_point]] main
.label main
    atom $1, @hello_world
    ebreak

    frame $1.a
    move $0.a, $1.l
    call void, dummy

    return void

.symbol [[local]] dummy
.label dummy
    ebreak
    return void
