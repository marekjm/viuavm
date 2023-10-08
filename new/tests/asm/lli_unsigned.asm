.section ".text"

.symbol [[entry_point]] main
.label main
    luiu $1, 0xffffffff
    lli $1, 0xffffffff

    ebreak
    return
