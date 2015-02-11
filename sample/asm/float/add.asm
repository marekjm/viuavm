; This file test support for fadd instruction and
; support for negative numbers in fstore.
; The nagative-numbers thingy is just nice-to-have and not the
; true porpose of this script, though.

.def: main 0
    fstore 1 4.0
    fstore 2 -3.5
    fadd 1 2 3
    fstore 4 0
    fadd 3 4
    print 3
    end
.end
