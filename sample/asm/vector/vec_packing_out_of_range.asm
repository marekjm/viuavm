.function: main/0
    string %2 "answer to life"
    integer %3 42

    -- this would pack outside of available
    -- register %set index range
    vec %1 %2 %20

    print %1

    izero %0 local
    return
.end
