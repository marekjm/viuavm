; This script tests support for float greater-than-or-equal-to checking.
; Its expected output is "true".

.def: main 0
    fstore 1 4.0
    fstore 2 3.99
    fgte 1 2
    print 1
    izero 0
    end
.end
