; In OCaml:
;
;   let rec fib n =
;       match n with
;       | 0 | 1 -> 1
;       | x -> (fib (x - 1)) + (fib (x - 2))
;


.function: fib/1
    allocate_registers %6 local

    .name: iota n

    move %n local %0 parameters

    .name: iota zero
    .name: iota one
    izero %zero local
    integer %one local 1

    eq %zero local %zero local %n local
    if %zero local +1 recursive_fib_calculation

    move %0 local %one local
    ; Uncomment either the return, or the jump to fix the "endless recursion
    ; detected" error thrown by the static analyser.
    ; ----
    ; return
    ; jump after_if

    .mark: recursive_fib_calculation

    .name: iota a
    .name: iota b

    ; Calculate the fib(n - 1) part.
    frame %1 arguments
    idec %n local
    copy %0 arguments %n local
    call %a local fib/1

    ; Calculate the fib(n - 2) part.
    frame %1 arguments
    idec %n local
    copy %0 arguments %n local
    call %b local fib/1

    ; Add the two parts together to make the final result.
    add %0 local %a local %b local

    .mark: after_if

    return
.end
