.function: main
    .name: 1 zero
    .name: 2 one

    ; store zero and one in registers
    istore zero 0
    istore one 1

    ; swap objects so that register "zero" now contains 1, and
    ; register "one" contains 0
    swap zero one

    ; print 0 and 1
    ; this should actually print 1 and 0
    print zero
    print one

    izero 0
    end
.end
