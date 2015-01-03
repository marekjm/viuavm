; purpose of this program is to compute n-th power of a number

; in first register we store the base number
istore 1 4

; in second register we store the power
istore 2 3

; store zero - we need it to compare the power, and as a countr for loop
istore 3 0

; if the power is equal to zero, store 1 in first register and jump to print
ieq 2 3 4
branchif 4 5 7
istore 6 1
branch 15

; now, we multiply in loop
istore 5 1
; in register 6, store the integer that can be found in register 1
istore 6 @1
ilt 5 2 4
branchif 4 11 15
imul 1 6 6
pass
iinc 5
branch 9

print 6
halt
