.function: main/0
    strstore %1 "answer to life"
    istore %2 42

    -- this would pack
    -- the vector inside itself
    vec %3 %1 %3

    print %3

    izero %0 local
    return
.end
