.section ".text"

.symbol [[entry_point]] main
.label main
    ; This value requires a multiplier to load, but has no high bits set. It
    ; broke the algorithm used before 2022-03-22.
    li $1, 19952203

    ebreak
    return
