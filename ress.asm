.def: main 0
    ; store 42 in second register of local register set
    istore 2 42
    
    ; switch to global register set
    ress global
    print 1
    ;print 2

    ; switch back to local
    ress local
    print 2
    end
.end


; this goes to global scope because of implicit "ress global" and
istore 1 42

; this ress will do noting as local registers of __entry function
; point to global register set
;ress local
;istore 2 69

frame
call main

;print 2
halt
