.section ".text"

; Make this function visible only on the first linking, as the linker will
; change STB_GLOBAL+STV_HIDDEN to STB_LOCAL+STV_DEFAULT.
;
; The purpose of global hidden symbols is to allow spliting a single module into
; multiple compilation (or, rather, assembly) units.
.symbol [[hidden]] dummy
.label dummy
    atom $1.l, hello_world
    ebreak
    return
