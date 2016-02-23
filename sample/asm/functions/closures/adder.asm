.function: adder
    ; expects register 1 to be an enclosed integer
    arg 2 0
    iadd 0 2 1
    return
.end

.function: make_adder
    ; takes an integer and returns a function of one argument adding
    ; given integer to its only parameter
    ;
    ; example:
    ;   make_adder(3)(5) == 8
    .name: 1 number
    arg number 0
    closure 2 adder
    enclose 2 1 1
    move 0 2
    return
.end

.function: main
    ; create the adder function
    .name: 2 add_three
    frame ^[(param 0 (istore 1 3))]
    call add_three make_adder

    ; add_three(2)
    frame ^[(param 0 (istore 3 2))]
    print (fcall 4 add_three)

    ; add_three(5)
    frame ^[(param 0 (istore 3 5))]
    print (fcall 4 add_three)

    ; add_three(13)
    frame ^[(param 0 (istore 3 13))]
    print (fcall 4 add_three)

    izero 0
    return
.end
