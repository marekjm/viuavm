.function: main
    vinsert (vinsert (vinsert (vinsert (vec 1) (strstore 5 "sheep!")) (strstore 2 "Hurr")) (strstore 3 "durr") 1) (strstore 4 "Im'a") 2

    ; this works OK
    ; this instruction is here for debugging - uncomment it to see the vector
    ;print 1

    .name: 6 len
    .name: 7 counter

    istore counter 0
    vlen len 1

    .mark: loop
    branch (ilt 8 counter len) +1 break
    print (vat 9 1 @counter)
    iinc counter
    jump loop

    .mark: break

    ; see comment for first vector-print
    ; it is for debugging - this should work without segfault
    ;print 1

    izero 0
    return
.end
