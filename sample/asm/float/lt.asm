; This script tests support for float less-than checking.
; Its expected output is "true".

.def: main 0
    fstore 1 1.00098
    fstore 2 1.00099
    flt 1 2 3
    print 3
    end
.end
