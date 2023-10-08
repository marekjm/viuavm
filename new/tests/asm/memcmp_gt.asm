.section ".rodata"

.label s1
.object string "Foo"

.label s2
.object string "Bar"

.section ".text"

.symbol [[extern]] "std::memcpy"
.symbol [[extern]] "std::memcmp"

.symbol [[entry_point]] main
.label main
    ; Load address of the first string.
    arodp $1.l, @s1

    ; Get size of the string.
    li $2.l, -8
    add $2.l, $1.l, $2.l
    ld $3.l, $2.l, 0
    cast $3.l, uint

    ; Allocate memory to hold the string.
    amba $4.l, $3.l, 0

    frame $3.a
    copy $0.a, $4.l ; dest
    copy $1.a, $1.l ; src
    copy $2.a, $3.l ; size
    call void, "std::memcpy"

    ; Load address of the second string.
    arodp $5.l, @s2

    ; Get size of the string in .rodata
    li $1.l, -8
    add $1.l, $5.l, $1.l
    ld $6.l, $1.l, 0
    cast $6.l, uint

    ; Allocate memory to hold the string.
    amba $7.l, $6.l, 0

    frame $3.a
    copy $0.a, $7.l ; dest
    copy $1.a, $5.l ; src
    copy $2.a, $6.l ; size
    call void, "std::memcpy"

    frame $3.a
    copy $0.a, $4.l ; first string
    copy $1.a, $7.l ; second string
    copy $2.a, $6.l ; size
    call $8.l, "std::memcmp"

    ebreak
    return void
