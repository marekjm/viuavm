.function: main/0
    integer (.name: %iota three) local 3
    float (.name: %iota pi) local 3.14
    float (.name: %iota almost_pi) local 3.13

    .name: iota result

    ; Converts value taken from `%pi local' to an integer.
    lt %result local %three local %pi local
    print %result local

    ; Does not perform any conversions.
    ; Both operands are floats.
    lt %result local %almost_pi local %pi local
    print %result local

    ; Converts value taken from `%three local` to a float.
    lt %result local %pi local %three local
    print %result local
    
    izero %0 local
    return
.end
