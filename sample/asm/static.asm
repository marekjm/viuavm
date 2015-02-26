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

    ; these two instructions are executed only when 1 register was null
    istore 1 42
    jump :report

    .mark: increase
    iinc 1

    .mark: report
    print 1

    istore 3 10
    ; integer at 1 is less than ten
    ilt 1 3 3
    ; integer at 1 is *at least* ten
    not 3
    branch 3 
    end
.end

.def: main 1
    istore 1 0
    istore 2 10

    .mark: loop
    ilt 1 2 3
    not 3

    branch 3 :finish
    frame 0
    call counter 0
    iinc 1
    jump :loop

    .mark: finish
    izero 0
    end
.end
