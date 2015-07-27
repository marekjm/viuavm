.function: print_number_of_params
    print (argc 1)
    end
.end

.function: main
    print (argc 1)

    izero 2

    frame 2
    param 0 2
    param 1 2
    call print_number_of_params

    frame 0
    call print_number_of_params

    izero 0
    end
.end
