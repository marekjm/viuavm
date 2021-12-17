.function: [[entry_point]] main
    li $1, 1u
    li $2, 10u

    ; 2nd instruction
    gt $3, $1, $2
    if $3, 9

    ; These two instructions are here just to test whether the calculations that
    ; the assembler makes to conver logical indexes into physical indexes are
    ; correct. Why these two? Because li with big numbers expands to more than
    ; one instruction.
    g.li $4, 318736561391831
    delete $4

    ebreak
    addi $1, $1, 1u
    jump 2

    ; 9th instruction
    ebreak
    return
.end
