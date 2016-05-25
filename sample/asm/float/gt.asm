; This script tests support for float greater-than checking.
; Its expected output is "true".

.function: main/1
    fstore 1 3.99
    fstore 2 3.98
    fgt 1 1 2
    print 1
    izero 0
    return
.end
