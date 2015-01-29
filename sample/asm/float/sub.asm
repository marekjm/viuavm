; This script tests float subtraction.

.def: main 0
    fstore 1 3.098
    fstore 2 2.083
    fsub 1 2 3
    print 3
    end
.end

frame 0
call main
halt
