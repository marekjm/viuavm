; This script tests whether integers can be freely used as conditions for branch instruction.
; Basically, it justs tests correctness of the .boolean() method override in Integer objects.

.def: main 0
    istore 1 1

    ; generate false
    istore 2 0
    istore 3 1
    ieq 2 3

    ; check
    branch 1 :ok :fin
    .mark: ok
    not 2
    .mark: fin
    print 2
    end
.end

frame 0
call main
halt
