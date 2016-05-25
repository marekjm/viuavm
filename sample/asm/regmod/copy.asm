; This script checks if the copy instruction works correctly.
; The instruction should copy the value of one register into another.
.function: main/1
    istore 1 1
    copy 2 1
    print 2
    izero 0
    return
.end
