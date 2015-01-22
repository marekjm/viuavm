; Test support for idec instruction.

.def: main 0
    istore 1 2
    idec 1
    print 1
    end
.end

call main
halt
