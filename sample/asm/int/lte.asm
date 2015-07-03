; This script tests support for integer less-than-or-equal-to checking.
; Its expected output is "true".

.function: main
    istore 1 2
    istore 2 1
    ilte 3 2 1
    print 3
    izero 0
    end
.end
