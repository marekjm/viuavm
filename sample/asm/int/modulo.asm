.def: main 0
    .name: 1 number
    .name: 2 modulo
    .name: 3 result

    istore number 166737
    istore modulo 176

    istore result @number

    ; if number is less than modulo, jump straight to result printing
    ilt number modulo 4
    branch 4 final

    ; otherwise we must perform some calculations
    idiv number modulo
    imul number modulo
    isub result number


    .mark: final
    print result
    izero 0
    end
.end
