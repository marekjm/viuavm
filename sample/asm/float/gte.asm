; This script tests support for float greater-than-or-equal-to checking.
; Its expected output is "true".

.function: main/1
    fstore 1 4.0
    fstore 2 3.99
    fgte 1 1 2
    print 1
    izero 0
    return
.end
