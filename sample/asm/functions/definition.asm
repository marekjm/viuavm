.function: foo
    istore 1 42
    print 1
    izero 0
    end
.end

; set alternative main function
.main: foo

; these instructions are just to prove that offsets work correctly
istore 1 0
iinc 1
ref 2 1
