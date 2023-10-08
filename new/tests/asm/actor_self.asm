.section ".text"

.symbol [[entry_point]] main
.label main
    self $1
    ebreak
    return
