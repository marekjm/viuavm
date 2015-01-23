.def: foo 0
    istore 1 42
    print 1
    end
.end

; these instructions are just to prove that offsets work correctly
istore 1 0
iinc 1
ref 1 2

jump :foo

; these will be skipped
istore 3 41
iadd 2 3
print 2

; this call should pring 42 to the console
.mark: foo
frame 0
call foo

halt
