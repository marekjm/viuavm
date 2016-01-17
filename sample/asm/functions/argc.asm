.function: print_number_of_params
    print (argc 1)
    return
.end

.function: main
    print (argc 1)

    izero 2

    frame ^[(param 0 2) (param 1 2)]
    call print_number_of_params

    call (frame 0) print_number_of_params

    izero 0
    return
.end
