.function: counter
    ; switch to static register set
    ress static

    ; check if 1 register is null
    isnull 2 1
    ; invert the check (check if register 1 is *not null*)
    not 2

    ; if register 1 is *not null* jump to :increase marker
    ; otherwise continue execution
    branch 2 increase

    ; these instructions are executed only when 1 register was null
    istore 1 0
    arg 3 0
    jump report

    .mark: increase
    iinc 1

    tmpri 1
    ress local
    tmpro 1

    arg 3 0

    ; integer at 1 is less than N
    ilt 4 1 3
    ; integer at 1 is *at least* N
    not 4
    branch 4 finish
    .mark: report
    print 1
    frame 1
    param 0 3
    call 0 counter

    .mark: finish
    end
.end

.function: main
    istore 1 10
    frame 1
    param 0 1
    call 0 counter

    izero 0
    end
.end
