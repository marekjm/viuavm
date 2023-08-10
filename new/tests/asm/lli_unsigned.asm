.function: [[entry_point]] main
    luiu $1, 0xffffffff0
    lli $1, 0xffffffff

    ebreak
    return
.end
