.label: s1
.value: string "Bar"

.label: s2
.value: string "Foo"

.function: [[extern]] "std::memcpy"
.function: [[extern]] "std::memcmp"

.function: [[entry_point]] main
    ; Load size of the first string.
    arodp $2.l, @s1
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

    ; Load size of the second string.
    arodp $5.l, @s2
    ld $6.l, $5.l, 0
    cast $6.l, uint

    ; Allocate memory to hold the string.
    amba $7.l, $6.l, 0

    ; Get address of the string in .rodata
    li $1.l, 8u
    add $1.l, $5.l, $1.l

    frame $3.a
    copy $0.a, $7.l
    copy $1.a, $1.l
    copy $2.a, $6.l
    call void, "std::memcpy"

    frame $3.a
    copy $0.a, $4.l
    copy $1.a, $7.l
    copy $2.a, $6.l
    call $8.l, "std::memcmp"

    ebreak
    return void
.end
