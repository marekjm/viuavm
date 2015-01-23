; This script tests support for integer equality checking.
; Its expected output is "true".

.def: main 0
    istore 1 1
    istore 2 1
    ieq 1 2

    ; should be true
    print 1
    end
.end

frame 0
call main
halt
