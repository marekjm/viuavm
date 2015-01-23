.def: rec 0
    idec 1
    ilt 1 2 3
    branch 3 :break_rec
    print 1
    frame 0
    call rec
    .mark: break_rec
    end
.end

istore 1 10
istore 2 0
frame 0
call rec
halt
