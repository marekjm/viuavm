; This script tests whether integers can be freely used as conditions for branch instruction.
; Basically, it justs tests correctness of the .boolean() method override in Integer objects.

.def: main 0
    istore 1 2
    branch 1 2 :fin
    idec 1
    .mark: fin
    print 1
    end
.end

frame 0
call main
halt
