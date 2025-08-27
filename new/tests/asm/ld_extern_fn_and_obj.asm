.section ".text"

.symbol [[extern]] fn

.symbol [[entry_point]] main
.label main
    frame $0.a
    call void, fn
    return
