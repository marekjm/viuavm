.function: counter
    ; switch to static register set
    ress static

    ; if register 1 is *not null* jump to increase marker
    ; otherwise continue execution to perform initial set up of static registers
    branch (not (isnull 2 1)) increase

    ; these instructions are executed only when 1 register was null
    ; they first setup static counter variable
    istore 1 0

    ; then, switch to local registers and...
    ress local
    ; 1) fake taking counter from static registers (it's zero during first pass anyway)
    istore 1 0
    ; 2) fetch the argument
    arg 3 0
    ; 3) jump straight to report mark
    jump report

    .mark: increase
    iinc 1

    tmpri 1
    ress local
    tmpro 1

    ; integer at 1 is *at least* N
    ; N is the parameter the function received
    branch (not (ilt 4 1 (arg 3 0))) finish

    .mark: report
    print 1
    frame ^[(param 0 3)]
    call 0 counter

    .mark: finish
    end
.end

.function: main
    frame ^[(param 0 (istore 1 10))]
    call ^(izero 0) counter
    end
.end
