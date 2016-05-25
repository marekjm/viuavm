; This script tests float multiplication.

.function: main/1
    fstore 1 4.0
    fstore 2 2.001
    fmul 3 1 2
    print 3
    izero 0
    return
.end
