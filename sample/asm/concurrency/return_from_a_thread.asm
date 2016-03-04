.function: run_in_a_thread
    istore 0 42
    return
.end

.function: main
    frame 0
    print (thjoin 2 (thread 1 run_in_a_thread))
    izero 0
    return
.end
