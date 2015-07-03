; This script tests support for float equality checking.
; Its expected output is "true".

.function: main
    fstore 1 0.69
    fstore 2 0.69
    feq 1 1 2

    ; should be true
    print 1
    izero 0
    end
.end
