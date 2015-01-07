; this is the first program written for Wudoo VM that combines
; instructions to carry out an action that is not hard coded as
; a standalone isntruction
;
; the purpose of this program is to find an absolute value of an integer

.name: 1 number
.name: 2 zero
.name: 3 is_negative

; store the int, of which we want to get an absolute value
istore number -17

; compare the int to zero
istore zero 0
ilt number zero is_negative

; if the int is less than zero, multiply it by -1
; else, branch directly to print instruction
branch is_negative 4 :final_print
istore 4 -1
imul 1 4

.mark: final_print
print number

halt
