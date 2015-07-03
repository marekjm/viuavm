; this is the first program written for Viua VM that combines
; instructions to carry out an action that is not hardcoded as
; a standalone isntruction
;
; the purpose of this program is to find an absolute value of an integer

.function: main
    .name: 1 number
    .name: 2 zero
    .name: 3 is_negative

    ; store the int, of which we want to get an absolute value
    istore number -17

    ; compare the int to zero
    istore zero 0
    ilt is_negative number zero

    ; if the int is less than zero, multiply it by -1
    ; else, branch directly to print instruction
    ; the negation of boolean is just to use short form of branch
    ; instruction - this construction starts emerging as a pattern...
    not is_negative
    branch is_negative final_print
    istore 4 -1
    imul 1 4

    .mark: final_print
    print number
    izero 0
    end
.end
