; This script checks if the copy instruction works correctly.
; The instruction should copy the value of one register into another.
.def: main 0
    end
.end

istore 1 1
copy 1 2
print 2
halt
