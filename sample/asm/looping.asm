; This script is just a simple loop.
; It displays numbers from 1 to 10.
istore 1 0
istore 2 10
.mark: loop
ilt 1 2 3
branch 3 4 7
print 1
iinc 1
jump loop
print 1
halt
