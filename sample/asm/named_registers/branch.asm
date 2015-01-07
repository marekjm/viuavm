.name: 1 condition
istore condition 1

; branch to :good if condition is true
; else branch to :bad
branch condition :good :bad

; the :good marker will output 1 before going to halt
.mark: good
print 1

; the :bad marker is just a halt instruction
.mark: bad
halt
