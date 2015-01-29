; This script's purpose is to stress register-reference syntax in
; integer related instructions.

.def: main 0
    istore 1 16
    istore 16 1

    print 1
    print @1
    print 16
    print @16
    end
.end

frame 0 17
call main
halt
