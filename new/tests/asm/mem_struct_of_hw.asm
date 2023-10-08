.section ".text"

.symbol [[entry_point]] main
.label main
    ; The struct looks like thus:
    ;
    ;   struct foo {
    ;       uint16_t a;
    ;       uint32_t b;
    ;   };
    ;
    ; What is sizeof(foo)? Two words ie, 8 bytes.
    ;
    ; Why, if a half-word and a word is only 6 bytes? Because the second field
    ; is a word, which needs to be properly aligned, thus creating a two byte
    ; gap beteen the first and second member.
    ;
    ; Why is this gap needed? Because the multiplier (the third parmeter of SM
    ; and LM instructions) works in increments of UNIT ie, two bytes for
    ; half-words, four bytes for words, etc. If we wanted the keep the order of
    ; the fields and remove the gap, accessing the second field would require
    ; using two SM instructions and some bit-shifting.

    ; Allocate space for a struct: two words.
    li $1.l, 2u
    amwa $1.l, $1.l, 0

    ; Create the value for the first memer of the struct, a half-word, and store
    ; it in the memory area allocated for the struct.
    li $2.l, 0xadde
    sh $2.l, $1.l, 0

    ; Do the same for the second member, a word.
    li $3.l, 0xefbeedfe
    sw $3.l, $1.l, 1

    ebreak
    return
