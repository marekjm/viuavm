.function: std::vector::of_ints/1
    ; Returns a vector of integers from 0 to N-1.
    ;
    ; N is received as first and only parameter
    ;
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

.function: std::vector::reverse/1
    ; Returns reversed vector.
    ;
    ; This function expects to receive its parameter by copy.
    ; Vector passed as the parameter is emptied.
    ; Reversing is *NOT* performed in-place.
    ;
    .name: 1 source
    .name: 0 result
    arg source 0
    vec result

    .name: 2 counter
    vlen counter source

    .mark: begin_loop
    .name: 3 tmp
    vpush result (vpop tmp source)
    branch (idec counter) begin_loop
    .mark: end_loop

    return
.end

.function: std::vector::reverse_in_place/1
    ; Returns vector reversed in-place.
    ;
    ; This function expects to receive its parameter by move (to avoid
    ; copying).
    ;
    .name: 0 source
    arg source 0

    .name: 1 counter_down
    vlen counter_down source
    idec counter_down
    .name: 2 counter_up
    izero counter_up
    .name: 3 limit
    copy limit counter_down

    .mark: begin_loop
    .name: 4 tmp
    vpop tmp source
    vinsert source tmp @counter_up

    idec limit
    idec counter_down
    iinc counter_up

    branch limit begin_loop
    .mark: end_loop

    return
.end

.function: std::vector::every/2
    ; Returns true if every element of the vector passes a test (supplied as a function in second parameter), false otherwise.
    ;
    .name: 1 vector
    .name: 2 fn
    arg vector 0
    arg fn 1

    .name: 0 result
    not (izero result)

    .name: 3 limit
    .name: 4 index
    vlen limit vector
    izero index

    ; do not loop on zero-length vectors
    branch limit +1 end_loop

    .mark: begin_loop
    .name: 5 tmp
    vpop tmp vector @index
    ; FIXME: there should be no copy operation - use pass-by-move instead
    frame ^[(param 0 tmp)]
    and result (fcall 6 fn) result

    vinsert vector tmp @index

    ; break loop if there wasn't a match
    branch result +1 end_loop

    branch (igte 5 (iinc index) limit) +1 begin_loop
    .mark: end_loop

    return
.end
