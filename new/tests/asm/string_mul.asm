.section ".rodata"

; Leave the string broken to test concatenation.
.label some_string
.object string "He" 2 * "l" "o, World!"

.section ".text"

.symbol [[extern]] "std::memcpy"

.symbol [[entry_point]] main
.label main
    ; Load address of the string.
    arodp $1.l, @some_string

    ; Get size of the string.
    li $2.l, -8
    add $2.l, $1.l, $2.l
    ld $3.l, $2.l, 0
    cast $3.l, uint

    ; Allocate memory to hold the string.
    amba $4.l, $3.l, 0

    ; Get address of the string in .rodata
    ;li $1.l, 8u
    ;add $1.l, $2.l, $1.l

    frame $3.a
    copy $0.a, $4.l
    copy $1.a, $1.l
    copy $2.a, $3.l
    call void, "std::memcpy"

    ebreak
    return void
