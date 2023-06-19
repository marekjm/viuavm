.label: some_string
.value: string "Hello, World!\n"

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
    call void, memcpy

    ; Create a write I/O request.
    ; It is a struct which looks like this:
    ;
    ;   struct {
    ;       u8  opcode;
    ;       u64 fd;
    ;       u64 buf_size;
    ;       u64 buf_addr;
    ;   };
    li $5.l, 4u
    amda $5.l, $5.l, 0

    ; Setup opcode.
    li $6.l, 1u
    sh $6.l, $5.l, 0

    ; Setup file descriptor (standard output).
    li $6.l, 1u
    sd $6.l, $5.l, 1

    ; Setup buffer size and address.
    sd $3.l, $5.l, 2
    sd $4.l, $5.l, 3

    ; FIXME No handle for the I/O request is produced.
    io_submit void, $5.l, void

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
