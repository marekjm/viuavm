.section ".rodata"

.label some_string
.object string "Hello, World!\n"

.section ".text"

.symbol [[extern]] "std::memcpy"

.symbol [[entry_point]] main
.label main
    ; Get address of the string.
    arodp $1.l, @some_string

    ; Load size of the string.
    li $2.l, -8
    add $2.l, $1.l, $2.l
    ld $2.l, $2.l, 0
    cast $2.l, uint

    ; Allocate memory to hold the string.
    amba $3.l, $2.l, 0

    frame $3.a
    copy $0.a, $3.l ; dest
    copy $1.a, $1.l ; src
    copy $2.a, $2.l ; size
    call void, "std::memcpy"

    ; Create a write I/O request.
    ; It is a struct which looks like this:
    ;
    ;   struct {
    ;       u16 opcode;
    ;       u64 fd;
    ;       u64 buf_size;
    ;       u64 buf_addr;
    ;   };
    ;
    ; We allocate four doublewords because the fields are aligned (thus the
    ; first u16 actually uses eight instead of two bytes).
    li $5.l, 4u
    amda $5.l, $5.l, 0

    ; Setup opcode: write.
    li $6.l, 1u
    sh $6.l, $5.l, 0

    ; Setup file descriptor (standard output).
    li $6.l, 1u
    sd $6.l, $5.l, 1

    ; Setup buffer size.
    sd $2.l, $5.l, 2

    ; Setup buffer address.
    ; FIXME Points to stack. Using .rodata directly does not work.
    sd $3.l, $5.l, 3

    ; Echo standard input to standard output.
    io_submit $7.l, $5.l, void
    io_wait void, $7.l, void

    ebreak
    return void
