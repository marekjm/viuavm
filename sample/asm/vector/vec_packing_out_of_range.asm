.function: main/0
    strstore %2 "answer to life"
    istore %3 42

    -- this would pack outside of available
    -- register %set index range
    vec %1 %2 %20

    print %1

    izero %0 local
    return
.end
