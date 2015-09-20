; std::random::randint/2(Integer lower_bound, Integer upper_bound) -> Integer
;   Returned integer is between lower and upper bound.
.signature: std::random::randint

; SYNOPSIS
;
;   viua-cpu a.out [lower-bound[, upper-bound]]
;
; DESCRIPTION
;
;   Program returns a random integer between upper and lower bound.
;   First given operand sets lower bound, second sets the upper one.
;   Default range is [0, 100).
;
; EXAMPLES
;
;   Return random integer between 0 and 100:
;
;       $ viua-cpu a.out
;       42
;
;   Return random integer between 42 and 127:
;
;       $ viua-cpu a.out 42 127
;       69
;
;   Return random integer between 40 and 100:
;
;       $ viua-cpu a.out 40
;       42
;
.function: main
    ; first, import the random module to make std::random functions available
    import "random"

    ; extract commandline operands vector
    arg 1 0
    vlen 2 1

    ; extract lower bound if 2 or more operands were given, default to 0 otherwise
    istore 3 2
    branch (igte 5 2 3) +1 default_lower_bound
    idec 3
    stoi 4 (vat 3 1 @3)
    ; register holding object obtained from vector must be emptied before reuse
    move (empty 3) 4
    ; jump two instructions to skip default lower bound assignment
    jump +2

    .mark: default_lower_bound
    istore 3 0

    ; extract upper bound if 3 or more operands were given, default to 100 otherwise
    istore 4 3
    branch (igte 5 2 4) +1 default_upper_bound
    idec 4
    stoi 5 (vat 4 1 @4)
    ; register holding object obtained from vector must be emptied before reuse
    move (empty 4) 5
    ; jump two instructions to skip default upper bound assignment
    jump +2

    .mark: default_upper_bound
    istore 4 100

    ; random integer between lower and upper bound
    frame ^[(param 0 3) (param 1 4)]
    print (call 6 std::random::randint)

    izero 0
    end
.end
