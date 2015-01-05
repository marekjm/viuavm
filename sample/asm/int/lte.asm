; This script tests support for integer less-than-or-equal-to checking.
; Its expected output is "true".
istore 1 2
istore 2 1
ilte 2 1 3
print 3
halt
