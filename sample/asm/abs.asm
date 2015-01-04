; this is the first program written for Wudoo VM that combines
; instructions to carry out an action that is not hard coded as
; a standalone isntruction
;
; the purpose of this program is to find an absolute value of an integer

; store the int, of which we want to get an absolute value
istore 1 -17

; compare the int to zero
istore 2 0
ilt 1 2 3

; if the int is less than zero, multiply it by -1
; else, branch directly to print instruction
branch 3 4 6
istore 4 -1
imul 1 4

print 1

halt
