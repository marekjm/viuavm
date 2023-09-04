.function: [[extern]] "std::memset"
.function: [[extern]] "std::memcpy"

.function: [[entry_point]] main
    ; size of buffer
    li $3, 16u

    ; byte to use to fill the buffer
    li $2, 0xffu

    ; allocate space for first copy
    copy $1, $3
    amba $1.l, $1.l, 0

    ; initialise first copy
    ; memset(3): (void*, int, size_t)
    frame $3.a
    copy $0.a, $1.l
    move $1.a, $2.l
    copy $2.a, $3.l
    call $4.l, "std::memset"

    ; allocate space for second copy
    li $2, 16u
    amba $2, $2, 0

    ; make the copy
    ; memcpy(3): (void* dst, void* src, size_t)
    frame $3.a
    move $0.a, $2.l
    move $1.a, $1.l
    move $2.a, $3.l
    call $5.l, "std::memcpy"

    ebreak
    return
.end
