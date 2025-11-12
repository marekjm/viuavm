.section ".text"

.symbol [[entry_point]] main
.label main
    li $1.l, 0

    ; There are several fundamental datatypes that we need to consider in the
    ; ALU tests:
    ;
    ;   - i64: signed integer
    ;   - u64: unsigned integer
    ;   - flt: floating point
    ;   - dbl: floating point (double)
    ;   - ptr: pointer
    ;   - atom: atom
    ;   - pid: PID
    ;
    ; A test case is always based on a primary data type eg, signed integer (as
    ; this case is), and a single operation eg, equality. The primary data type
    ; appears as the left-hand operand, and all the other data types appear as
    ; the right-hand side operand.
    ;
    ; Of course, incompatible operand types are skipped. This concerns pointers,
    ; atoms, PIDs, and other non-arithmetic types since they are a bit special
    ; and can only be compared to other values of the same type - the virtual
    ; machine does not perform any automatic conversions. (A determined user can
    ; use memory to launder the values, but this requires the provenance checks
    ; to be disabled.)
    ;
    ; The format for names of test cases is: arnv_cmp_<op>_<type>, where
    ;
    ;   - <op> is the operation
    ;   - <type> is one of the fundamental types
    ;
    ; The <op> part is one of the following:
    ;
    ;   - eq:  equals
    ;   - lt:  less-than
    ;   - gt:  greater-than
    ;
    ; All the other comparisons can be synthesised from the above. For example,
    ; a "greater-than or equal" comparison can be written as:
    ;
    ;       eq $eq, $lhs, $rhs
    ;       lt $lt, $lhs, $rhs
    ;       or $result, $eq, $lt
    ;
    ; The eq, lt, and gt operations return an unsigned integer flag representing
    ; the result: 1 if the comparison yields truth, 0 otherwise.

    li $2.l, 0
    eq $3.l, $1.l, $2.l

    li $2.l, 42
    eq $4.l, $1.l, $2.l

    li $2.l, 0u
    eq $5.l, $1.l, $2.l

    li $2.l, 42u
    eq $6.l, $1.l, $2.l

    float $2.l, 0.0
    eq $7.l, $1.l, $2.l

    float $2.l, 3.14
    eq $8.l, $1.l, $2.l

    double $2.l, 0.0
    eq $9.l, $1.l, $2.l

    double $2.l, 3.14159
    eq $10.l, $1.l, $2.l

    ebreak
    return
