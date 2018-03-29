.function: main/0
    string %2 local "answer to life"
    integer %3 local 42

    -- this would pack outside of available
    -- register %set index range
    vector %1 local %2 local %20

    print %1 local

    izero %0 local
    return
.end
