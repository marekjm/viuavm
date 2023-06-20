.label: some_string
.value: string "Hello, World!\n"

.function: [[entry_point]] main
    ; Allocate memory to hold the string.
    li $3.l, 100u
    amba $4.l, $3.l, 0

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

    ; Setup opcode: read.
    li $6.l, 0u
    sh $6.l, $5.l, 0

    ; Setup file descriptor (standard input).
    li $6.l, 0u
    sd $6.l, $5.l, 1

    ; Setup buffer size and address.
    sd $3.l, $5.l, 2
    sd $4.l, $5.l, 3

    ; Submit the request and wait for it to complete.
    ; The program will be suspended until some data arrives on standard input
    ; stream.
    io_submit $7.l, $5.l, void
    io_wait $8.l, $7.l, void

    ; Load the size of actually read data.
    ld $9.l, $8.l, 2
    cast $9.l, uint

    ; Setup opcode: write.
    li $6.l, 1u
    sh $6.l, $5.l, 0

    ; Setup file descriptor (standard output).
    li $6.l, 1u
    sd $6.l, $5.l, 1

    ; Setup buffer size and address.
    sd $9.l, $5.l, 2

    ; Echo standard input to standard output.
    io_submit $7.l, $5.l, void
    io_wait $10.l, $7.l, void

    ebreak
    return void
.end
