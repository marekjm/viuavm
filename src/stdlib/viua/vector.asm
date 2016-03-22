.function: std::vector::of_ints/1
    ; returns a vector of integers from 0 to N-1
    ; N is received as first and only parameter
    .name: 1 limit
    arg limit 0

    .name: 0 vector
    vec vector

    .name: 3 counter
    .name: 4 to_push
    izero counter

    .mark: begin_loop
    vpush vector (copy to_push counter)
    iinc counter
    ; reuse 'to_push' register since it's empty
    branch (igte to_push counter limit) +1 begin_loop

    return
.end
