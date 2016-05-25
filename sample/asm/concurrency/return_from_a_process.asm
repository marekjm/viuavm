.function: run_in_a_process/0
    istore 0 42
    return
.end

.function: main/1
    frame 0
    print (join 2 (process 1 run_in_a_process/0))
    izero 0
    return
.end
