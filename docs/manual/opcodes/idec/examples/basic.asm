.function: main/0
    integer %1 local 43

    ; prints 43
    print %1 local

    ; increments integer at 1st local register
    idec %1 local

    ; prints 42
    print %1 local

    izero %0 local
    return
.end
