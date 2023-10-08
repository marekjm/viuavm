.section ".rodata"

.symbol [[extern]] object

.section ".text"

.symbol [[entry_point]] main
.label main
    atom $1.l, @object
    ebreak
    return
