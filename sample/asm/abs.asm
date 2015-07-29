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

    ; if the int is less than zero, multiply it by -1
    ; else, branch directly to print instruction
    ; the negation of boolean is just to use short form of branch
    ; instruction - this construction starts emerging as a pattern...
    branch (not (ilt is_negative number (istore zero 0))) final_print
    imul 1 (istore 4 -1)

    .mark: final_print
    print number
    izero 0
    end
.end
