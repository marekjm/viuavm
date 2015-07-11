.function: print_number_of_params
    argc 1
    print 1
    end
.end

.function: main
    argc 1
    print 1

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
