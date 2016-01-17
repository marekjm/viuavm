; This script tests support for float less-than-or-equal-to checking.
; Its expected output is "true".

.function: main
    fstore 1 1.098
    fstore 2 1.099
    flte 3 1 2
    print 3
    izero 0
    return
.end
