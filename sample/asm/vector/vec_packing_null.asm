.function: main/0
    string %2 "answer to life"
    integer %3 42

    -- this would pack
    -- the vector inside itself
    vector %1 %2 %8

    print %1

    izero %0 local
    return
.end
