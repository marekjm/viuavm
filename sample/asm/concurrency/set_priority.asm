.function: run_in_a_process/0
.name: 1 counter
.name: 2 limit
    izero counter
    istore limit 32
.mark: loop
    branch (igte 4 counter limit) endloop +1
    ; execute nops to make the process longer
    nop
    iinc counter
    jump loop
.mark: endloop

    print (strstore 5 "Hello concurrent World!")
    return
.end

.function: main
    frame 0
    process 1 run_in_a_process/0

    frame ^[(param 0 1) (param 1 (istore 2 40))]
    msg 0 setPriority/2

    frame ^[(param 0 1)]
    print (msg 2 getPriority/1)
.name: 3 counter
.name: 4 limit
    izero counter
    ; set lower limit than for spawned process to force
    ; the priority settings take effect
    istore limit 16
.mark: loop
    branch (igte 5 counter limit) endloop +1
    ; execute nops to make main process do something
    nop
    iinc counter
    jump loop
.mark: endloop

    print (strstore 6 "Hello sequential World!")

    join 0 1

    izero 0
    return
.end
