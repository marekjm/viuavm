.label: some_string
.value: string "Hello, World!"

.function: [[extern]] "std::memcpy"

.function: [[entry_point]] main
    ; string $1, "Hello, World!"

    ; Load size of the string.
    arodp $2.l, @some_string
    ld $3.l, $2.l, 0
    cast $3.l, uint

    ; Allocate memory to hold the string.
    amba $4.l, $3.l, 0

    ; Get address of the string in .rodata
    li $1.l, 8u
    add $1.l, $2.l, $1.l

    frame $3.a
    copy $0.a, $4.l
    copy $1.a, $1.l
    copy $2.a, $3.l
    call void, "std::memcpy"

    ebreak
    return void
.end
