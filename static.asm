.def: counter 0
    ; switch to static register set
    ress static

    ; check if 1 register is null
    isnull 1 2
    ; invert the check (check if register 1 is *not null*)
    not 2

    ; if register 1 is *not null* jump to :increase marker
    ; otherwise continue execution
    branch 2 :increase

    ; these instructions are executed only when 1 register was null
    istore 1 0
    arg 0 3
    jump :report

    .mark: increase
    iinc 1

    tmpri 1
    ress local
    tmpro 1

    arg 0 3

    ; integer at 1 is less than N
    ilt 1 3 4
    ; integer at 1 is *at least* N
    not 4
    branch 4 :finish
    .mark: report
    print 1
    frame 1
    param 0 3
    call counter 0

    .mark: finish
    end
.end

.def: main 1
    istore 1 3
    frame 1
    param 0 1
    call counter 0

    izero 0
    end
.end
