; purpose of this program is to compute n-th power of a number

.name: 1 base
.name: 2 exponent
.name: 3 zero
.name: 6 result

; store operands of the power-of operation
istore base 4
istore exponent 3

; store zero - we need it to compare the exponent to it
istore zero 0

; if the exponent is equal to zero, store 1 in first register and jump to print
ieq exponent zero 4

; invert so we can use short form of branch instruction
not 4
branch 4 :algorithm
istore result 1
jump :final_print

; now, we multiply in a loop
.mark: algorithm
.name: 5 counter
istore counter 1
; in register 6, store the base of power as
; we will need it for multiplication
istore result @base

.mark: loop
ilt counter exponent 4
branch 4 12 :final_print
; TODO: this imul should use short form
imul base result result
pass
iinc counter
jump :loop

; print the result
.mark: final_print
print result

; and halt the program
halt
