.function: main/1
    .name: 1 number
    .name: 2 modulo
    .name: 3 result

    istore number 166737
    istore modulo 176

    istore result @number

    ; if number is less than modulo, jump straight to result printing
    ilt 4 number modulo
    branch 4 final

    ; otherwise we must perform some calculations
    idiv number number modulo
    imul number number modulo
    isub result result number


    .mark: final
    print result
    izero 0
    return
.end
