.section ".text"

.symbol [[entry_point]] main
.label main
    ; The struct looks like thus:
    ;
    ;   struct foo {
    ;       uint32_t a;
    ;       uint16_t b;
    ;   };
    ;
    ; What is sizeof(foo)? Three half-words ie, 6 bytes.

    ; Allocate space for a struct: three half-words.
    li $1.l, 3u
    amha $1.l, $1.l, 0

    ; Create the value for the first memer of the struct, a word, and store it
    ; in the memory area allocated for the struct.
    li $3.l, 0xefbeedfe
    sw $3.l, $1.l, 0

    ; Do the same for the second member, a half-word.
    li $2.l, 0xadde
    sh $2.l, $1.l, 2

    ebreak
    return
