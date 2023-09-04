.label: s1
.value: string "Foo"

.label: s2
.value: string "Foo"

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
    call void, memcpy

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
    call void, memcpy

    frame $3.a
    copy $0.a, $4.l
    copy $1.a, $7.l
    copy $2.a, $6.l
    call $8.l, memcmp

    ebreak
    return void
.end

.function: memcpy
    li $1, 0u

    g.lt $2.l, $1.l, $2.p
    g.not $2.l, $2.l
    if $2.l, 10

    ; load from src
    add $3.l, $1.p, $1.l
    lb $2, $3, 0

    ; store to dst
    add $3, $0.p, $1.l
    sb $2, $3, 0

    ; increase counter and repeat
    addi $1, $1, 1u
    jump 1

    move $2.l, $0.p
    return $2.l
.end

.function: memcmp
    li $1, 0u
    li $8.l, 0

    g.lt $2.l, $1.l, $2.p
    g.not $2.l, $2.l
    if $2.l, 15

    ; load from s1
    add $3.l, $0.p, $1.l
    g.lb $4.l, $3.l, 0
    cast $4.l, uint

    ; load from s2
    add $5.l, $1.p, $1.l
    g.lb $6.l, $5.l, 0
    cast $6.l, uint

    g.cmp $8.l, $4.l, $6.l
    if $8.l, 15

    ; increase counter and repeat
    addi $1, $1, 1u
    jump 2

    return $8.l
.end
