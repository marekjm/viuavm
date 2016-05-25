.function: worker_process
    echo (strstore 1 "Hello from #")
    echo (arg 2 0)
    print (strstore 1 " worker process!")
    return
.end

.function: process_spawner
    echo (strstore 1 "number of worker processes: ")

    .name: 1 limit
    print (arg limit 0)

    .name: 3 counter
    izero counter

    .mark: begin_loop
    branch (igte 4 counter limit) end_loop +1

    frame ^[(param 0 counter)]
    process 2 worker_process

    frame ^[(param 0 (ptr 5 2))]
    msg 0 detach/1
    ; explicitly delete the pointer to process handle
    delete 5

    iinc counter
    jump begin_loop
    .mark: end_loop

    return
.end


; std::io::getline/0 is required to get user input
.signature: std::io::getline

.function: get_number_of_processes_to_spawn
    echo (strstore 4 "number of processes to spawn: ")
    frame 0
    stoi 0 (call 4 std::io::getline)
    return
.end

.function: run_process_spawner
    frame ^[(param 0 (arg 1 0))]
    process 2 process_spawner

    frame ^[(param 0 (ptr 3 2))]
    msg 0 detach/1

    return
.end

.function: main/1
    ; the standard "io" library contains std::io::getline
    import "io"

    frame 0
    call 4 get_number_of_processes_to_spawn

    frame ^[(param 0 4)]
    call run_process_spawner

    izero 0
    return
.end
