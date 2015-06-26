; This script tests support for integer less-than checking.
; Its expected output is "true".

.def: main
    istore 1 2
    istore 2 1
    ilt 2 1 3
    print 3
    izero 0
    end
.end
