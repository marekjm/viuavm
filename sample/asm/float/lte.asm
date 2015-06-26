; This script tests support for float less-than-or-equal-to checking.
; Its expected output is "true".

.def: main
    fstore 1 1.098
    fstore 2 1.099
    flte 1 2 3
    print 3
    izero 0
    end
.end
