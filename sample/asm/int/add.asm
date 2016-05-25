; This file test support for iadd instruction and
; support for negative numbers in istore.
; The nagative-numbers thingy is just nice-to-have and not the
; true porpose of this script, though.

.function: main/1
    istore 1 4
    istore 2 -3
    iadd 3 1 2
    istore 4 0
    iadd 3 3 4
    print 3
    izero 0
    return
.end
