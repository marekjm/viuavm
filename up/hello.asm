.import: [[static]] modfoo
.import: [[dynamic]] modbaz

.function: main/0
    allocate_registers %1 local

    frame %0 arguments
    call void modfoo::fn/0

    frame %0 arguments
    call void modbar::fn/0

    izero %0 local
    return
.end
