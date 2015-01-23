; This file test support for iadd instruction and
; support for negative numbers in istore.
; The nagative-numbers thingy is just nice-to-have and not the
; true porpose of this script, though.

.def: main 0
    istore 1 4
    istore 2 -3
    iadd 1 2 3
    istore 4 0
    iadd 3 4
    print 3
    end
.end

frame 0
call main
halt
