.def: foo 0
    istore 1 42
    print 1
    end
.end

.main: foo

; these instructions are just to prove that offsets work correctly
istore 1 0
iinc 1
ref 1 2

; this call should pring 42 to the console
frame 0
call foo

halt
