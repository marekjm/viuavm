; This script checks if the copy instruction works correctly.
; The instruction should copy the value of one register into another.
.def: main
    istore 1 1
    copy 1 2
    print 2
    izero 0
    end
.end
