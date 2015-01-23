; This script tests support for integer greater-than-or-equal-to checking.
; Its expected output is "true".

.def: main 0
    istore 1 4
    istore 2 1
    igte 1 2
    print 1
    end
.end

frame 0
call main
halt
