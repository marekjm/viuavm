.def: by_reference
    arg 0 0
    end
.end

.def: main
    ; store a number in register
    istore 1 69

    frame 1
    ; pass it by reference to the function
    paref 0 1
    ; store return value in another register (it is a reference!)
    call by_reference 2
    ; assign different value to it
    istore 2 42
    ; check if return-by-reference is working (should print 42)
    print 1

    izero 0
    end
.end
