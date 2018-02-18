.function: main/0
    integer %1 local 41

    ; prints 41
    print %1 local

    ; increments integer at 1st local register
    iinc %1 local

    ; prints 42
    print %1 local

    izero %0 local
    return
.end
