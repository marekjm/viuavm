; This script's purpose is to stress register-reference syntax in
; integer related instructions.

.function: foo/0
    istore 1 16
    istore 16 1

    print 1
    print @1
    print 16
    print @16

    return
.end

.function: main
    frame 0 17
    call foo/0
    izero 0
    return
.end
