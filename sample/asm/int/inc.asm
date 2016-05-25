; This script tests support for iinc i++. instruction, i.e.

.function: main/1
    istore 1 0
    iinc 1
    print 1
    izero 0
    return
.end
