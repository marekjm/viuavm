; This script is just a simple loop.
; It displays numbers from 0 to 10.
istore 1 0
istore 2 10

; mark the beginning of the loop
.mark: loop
ilt 1 2 3
; invert value to use short form of branch instruction, i.e.: branch <cond> <true>
; and expliot the fact that it will default false to "next instruction"
not 3
branch 3 :final_print
print 1
iinc 1
jump :loop
.mark: final_print
print 1
halt
