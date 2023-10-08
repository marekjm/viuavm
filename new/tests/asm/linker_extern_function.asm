.section ".text"

.symbol [[extern]] dummy

.symbol [[entry_point]] main
.label main
    frame $0.a
    call void, dummy
    return
