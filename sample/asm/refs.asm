.def: main 0
    .name: 1 number
    .name: 3 indirect_ref

    ; store 0 in register :number"
    istore number 0

    ; create a reference to register "number" in register 2
    ref number 2

    ; store 2 in register 2 and print number
    ; number will now be 2
    istore 2 2
    print number

    ; create reference to register 2 in register "indirect_ref"
    ; registers 1, 2 and 3 all now point to the same object
    ref 2 indirect_ref

    ; store
    istore indirect_ref 4

    ; multiply "number" by "indirect_ref" and
    ; store the result in register 2
    imul 2 indirect_ref 2

    ; print number only to learn it is neither 0 (like in the beginning)
    ; nor 2 (as istore to a direct reference may suggest)
    ; but that it is 16!
    print number
    izero 0
    end
.end
