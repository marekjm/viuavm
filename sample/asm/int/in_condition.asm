; This script tests whether integers can be freely used as conditions for branch instruction.
; Basically, it justs tests correctness of the .boolean() method override in Integer objects.
istore 1 2
branch 1 2 4
idec 1
print 1
halt
