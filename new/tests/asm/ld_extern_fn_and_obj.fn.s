.section ".rodata"

.symbol [[extern]] obj

.section ".text"

.symbol fn
.label fn
    atom $1.l, @obj
    ebreak
    return
