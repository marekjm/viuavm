.def: fibonacci_memoizing 1
    ; name the registers
    .name: 1 one
    .name: 2 two
    .name: 3 number
    .name: 4 result
    .name: 5 first
    .name: 6 second

    ; FIXME: remove this after debugging
    izero result

    ; set needed values
    istore one 1
    istore two 2

    ; get the argument
    arg 0 number

    ; check if we can skip calculations and
    ; just return 0
    izero 0
    ieq 0 number
    branch 0 give_zero

    ; check if we can skip calculations and
    ; just return 1
    ieq one number
    ieq two number
    or one two
    ; if number is one or two give_one as the result
    branch one give_one

    istore one 1
    istore two 2

    ; all checks failed so we have to actually calculate the
    ; number...

    ; calculate N-1
    isub number one one
    frame 1
    param 0 one
    call fibonacci_memoizing first

    tmpri one
    ress global
    tmpro 1
    isnull @1 2
    not 2
    branch 2 is_filled
    ; here, register is not filled - return value not memorized
    free 2
    ress local
    tmpri first
    ress global
    tmpro @1
    .mark: is_filled
    free 1
    free 2
    ress local

    ; calculate N-2
    isub number two two

    ; store N-2 in temporary register
    tmpri two
    ; switch to global registers
    ress global
    ; toss temporary register out, to 1 global register
    tmpro 1
    ; check if register N-2 is null (not calculated)
    isnull @1 2
    tmpri 2
    branch 2 nope
    .mark: nope
    ; switch back to local registers
    ress local
    tmpro 0

    branch 0 we_must_calculate fetch_it

    .mark: we_must_calculate
    ;.mark: fetch_it
    frame 1
    param 0 two
    call fibonacci_memoizing second
    jump add_them

    .mark: fetch_it
    ress global
    ; fetch from register 29 if we calculate 29
    tmpri @1
    free 1
    ress local
    ; drop fetched value to regsiter "two"
    tmpro second

    ; put (N-1) + (N-2) in result register
    .mark: add_them
    iadd first second result
    jump finish

    .mark: give_zero
    izero result
    jump finish

    .mark: give_one
    istore result 1

    .mark: finish
    move result 0
    end
.end


.def: main 1
    .name: 1 number
    .name: 2 expected
    .name: 3 result

    ; set Fibonacci number we want to calculate
    ; and expected value (for test)

    ;istore number 10
    ;istore expected 55

    ;istore number 20
    ;istore expected 6765

    ;istore number 30
    ;istore expected 832040

    ;istore number 35
    ;istore expected 9227465

    ;istore number 40
    ;istore expected 102334155

    istore number 45
    istore expected 1134903170

    ; 50. Fibonacci number is too big for an integer
    ;istore number 50
    ;istore expected 12586269025

    ; create appropriate frame
    frame 1
    param 0 number
    call fibonacci_memoizing result

    ; print out the result
    print result

    ; finish the function
    izero 0
    end
.end
