; this code will calculate Fibonacci numbers

.def: fib 1
    ; count n-th Fibonacci number
    arg 0 1

    ; check if N is one or two
    .name: 2 one
    .name: 3 two
    istore one 1
    istore two 2
    ieq 1 one 4
    ieq 1 two 5
    or 4 5
    not 4
    branch 4 :not_one_and_not_two
    istore 1 1
    ret 1
    end

    .mark: not_one_and_not_two

    ; count fib of N-1
    isub 1 one
    frame 1
    param 0 1
    call fib 3

    ; count fib of N-2
    isub 1 one
    frame 1
    param 0 1
    call fib 4

    ; add these two calls
    iadd 3 4 5
    ; and return the value
    ret 5

    end
.end


.def: main 0
    .name: 5 result
    .name: 4 n
    .name: 3 counter
    .name: 2 max
    .name: 1 check

    istore counter 16
    ; uncomment "max 32" to count numbers up to 32
    ; but be aware that it takes about a minute to calculate on Intel i5-430M
    istore max 16
    ;istore max 32

    .name: 6 space
    .name: 7 dot
    bstore 6 32
    bstore 7 46

    .mark: loop
    ilte counter max check

    branch check :inside :break

    .mark: inside
    istore n @counter

    ; uncomment these instructions to print "indexes" of Fibonacci numbers
    ;echo counter
    ;echo 7
    ;echo 6

    frame 1
    param 0 n
    call fib result
    print result

    iinc counter
    jump :loop

    .mark: break
    end
.end


frame
call main
halt
