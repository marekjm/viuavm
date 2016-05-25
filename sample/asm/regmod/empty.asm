; FIXME: remove this test - with removal of REF the EMPTY instruction is not that useful

.function: main/1
    ; check if register 1 is null
    print (isnull 2 1)

    izero 0
    return
.end
