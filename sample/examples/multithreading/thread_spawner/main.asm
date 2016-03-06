.function: worker_thread
    echo (strstore 1 "Hello from #")
    echo (arg 2 0)
    print (strstore 1 " worker process!")
    return
.end

.function: thread_spawner
    echo (strstore 1 "number of worker threads: ")

    .name: 1 limit
    print (arg limit 0)

    .name: 3 counter
    izero counter

    .mark: begin_loop
    branch (igte 4 counter limit) end_loop +1

    frame ^[(param 0 counter)]
    process 2 worker_thread
    frame ^[(paref 0 2)]
    msg 0 detach

    iinc counter
    jump begin_loop
    .mark: end_loop

    return
.end


; std::io::getline/0 is required to get user input
.signature: std::io::getline

.function: get_number_of_threads_to_spawn
    echo (strstore 4 "number of threads to spawn: ")
    frame 0
    stoi 0 (call 4 std::io::getline)
    return
.end

.function: run_thread_spawner
    frame ^[(param 0 (arg 1 0))]
    process 2 thread_spawner

    frame ^[(paref 0 2)]
    msg 0 detach

    return
.end

.function: main
    ; the standard "io" library contains std::io::getline
    import "io"

    frame 0
    call 4 get_number_of_threads_to_spawn

    frame ^[(param 0 4)]
    call run_thread_spawner

    izero 0
    return
.end
