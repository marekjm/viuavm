.section ".text"

.symbol [[entry_point]] main
.label main
    call void, dummy
    return

.symbol dummy
.label dummy
    li $1, 1u
    li $2, 10u

.label loop
    gt $3, $1, $2
    if $3, epilogue

    ; These two instructions are here just to test whether the calculations that
    ; the assembler makes to convert logical indexes into physical indexes are
    ; correct. Why these two? Because li with big numbers expands to more than
    ; one instruction.
    [[full]] g.li $4, 318736561391831
    delete $4

    ebreak
    addi $1, $1, 1u
    if void, loop

.label epilogue
    ebreak
    return
