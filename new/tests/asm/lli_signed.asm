.function: [[entry_point]] main
    lui $1, 0xffffffff0
    lli $1, 0xffffffff

    ebreak
    return
.end
