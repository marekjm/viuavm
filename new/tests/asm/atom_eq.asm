.section ".text"

.symbol [[entry_point]] main
.label main
    atom $1.l, foo
    atom $2.l, bar

    eq $3.l, $1.l, $1.l
    eq $4.l, $1.l, $2.l

    ebreak

    return void
