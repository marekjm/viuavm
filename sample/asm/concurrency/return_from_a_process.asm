.function: run_in_a_process
    istore 0 42
    return
.end

.function: main
    frame 0
    print (join 2 (process 1 run_in_a_process))
    izero 0
    return
.end
