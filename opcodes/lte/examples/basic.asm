.function: main/0
    integer (.name: %iota three) local 3
    float (.name: %iota pi) local 3.14
    float (.name: %iota almost_pi) local 3.13

    .name: iota result

    ; Converts value taken from `%pi local' to an integer.
    lte %result local %three local %pi local
    print %result local

    ; Does not perform any conversions.
    ; Both operands are floats.
    ; The result is `true' because 3.13 is less than or
    ; equal to 3.14.
    lte %result local %almost_pi local %pi local
    print %result local

    ; Does not perform any conversions.
    ; Both operands are floats.
    ; The result is `false' because 3.14 is not less than or
    ; equal to 3.13.
    lte %result local %pi local %almost_pi local
    print %result local

    ; Converts value taken from `%three local` to a float.
    lte %result local %pi local %three local
    print %result local
    
    izero %0 local
    return
.end
