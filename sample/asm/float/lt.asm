; This script tests support for float less-than checking.
; Its expected output is "true".

.function: main
    fstore 1 1.00098
    fstore 2 1.00099
    flt 3 1 2
    print 3
    izero 0
    return
.end
