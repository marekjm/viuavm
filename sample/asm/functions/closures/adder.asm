.function: adder
    arg 2 0
    iadd 0 2 1
    return
.end

.function: make_adder
    .name: 1 number
    arg number 0
    clbind 1
    closure 2 adder
    move 0 2
    return
.end

.function: main
    .name: 2 add_three
    frame ^[(param 0 (istore 1 3))]
    call add_three make_adder

    frame ^[(param 0 (istore 3 2))]
    print (fcall 4 add_three)

    frame ^[(param 0 (istore 3 5))]
    print (fcall 4 add_three)

    frame ^[(param 0 (istore 3 13))]
    print (fcall 4 add_three)

    izero 0
    return
.end
