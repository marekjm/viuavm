.function: [[entry_point]] main
    lui $1, 0xffffffff
    lli $1, 0xffffffff

    ebreak
    return
.end
