.function: main/0
    allocate_registers %4 local

    ;.name: iota stdin
    .name: iota stdout
    ;.name: iota stderr

    .name: iota value
    .name: iota ioreq
    .name: iota iores

    text %value local "⇚⇛⇜⇝⇠⇢⇡⇣⇤⇥↻⊗⇐⇑⇒⇓⇖⇘\n"
    integer %stdout local 1

    io_write %ioreq local %stdout local %value local
    io_wait void %ioreq local 1s

    izero %0 local
    return
.end
