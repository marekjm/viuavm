; This script tests support for iinc instruction, i.e. i++.

.def: main 0
    istore 1 0
    iinc 1
    print 1
    end
.end

call main
halt
