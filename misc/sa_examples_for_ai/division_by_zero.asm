.function: main/0
    allocate_registers %3 local

    .name: iota lhs
    .name: iota rhs

    integer %lhs local 1
    integer %rhs local 1    ; This value will be decremented later. It is
                            ; statically known so the result of decrementation
                            ; is also known.
                            ; This allows the SA to prove that the division will
                            ; be invalid.

    if %lhs local true_arm false_arm

    .mark: true_arm
    idec %rhs local
    jump end_if

    .mark: false_arm
    .mark: end_if
    div %lhs local %lhs local %rhs local

    izero %0 local
    return
.end
