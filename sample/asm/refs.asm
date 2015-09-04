.function: main
    .name: 1 number
    .name: 3 indirect_ref

    istore 1 0
    ref 2 1

    ; store 2 in register 2 and print number
    ; number will now be 2
    istore 2 2
    print 1

    ; create reference to register 2 in register "indirect_ref"
    ; registers 1, 2 and 3 all now hold references to the same object
    ref 3 2

    istore 3 4

    ; multiply "number" by "indirect_ref" and
    ; store the result in register 2
    imul 2 3 2

    ; print number only to learn it is neither 0 (like in the beginning)
    ; nor 2 (as istore to a direct reference may suggest)
    ; but that it is 16!
    print number

    izero 0
    end
.end
