; This script tests support for integer less-than checking.
; Its expected output is "true".

.function: main
    istore 1 2
    istore 2 1
    ilt 3 2 1
    print 3
    izero 0
    return
.end
