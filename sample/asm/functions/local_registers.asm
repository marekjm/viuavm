.def: main 0
    istore 1 42
    print 1
    end
.end

istore 1 69
frame 0
call main 0
; FIXME: CPU should remember register set a function used
; even after returning from a call
; for now this ress must be there
ress global
print 1
halt
