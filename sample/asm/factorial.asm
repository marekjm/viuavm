.def: factorial 0
    imul 2 1
    idec 1
    ; if counter is equal to zero
    ieq 1 3 4
    branch 4 :finish
    frame 0
    call factorial
    .mark: finish
    end
.end

.def: main 0
    .name: 1 number
    istore number 5
    istore 2 1
    istore 3 0
    frame 0
    call factorial
    print 2
    end
.end

frame 0
call main
halt
