.function: run_in_a_thread_lazy
    arg 1 0
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    echo (strstore 2 "Hello World! (from thread ")
    echo 1
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    print (strstore 2 ')')
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    end
.end

.function: run_in_a_thread_eager
    arg 1 0
    echo (strstore 2 "Hello World! (from thread ")
    echo 1
    nop
    nop
    print (strstore 2 ')')
    nop
    end
.end

.function: main
.name: 1 counter
.name: 2 limit
    izero counter
    ;istore limit 524288
    ;istore limit 262144
    ;istore limit 131072
    ;istore limit 65536
    ;istore limit 32768
    ;istore limit 16384
    istore limit 8192
    ;istore limit 6194
    ;istore limit 4096
    ;istore limit 2048
    ;istore limit 1024
    ;istore limit 512
    ;istore limit 256
    ;istore limit 128
    ;istore limit 64
    ;istore limit 32
    ;istore limit 16
    ;istore limit 8
    ;istore limit 4
    ;istore limit 2
.mark: loop
    branch (igte 3 counter limit) endloop

    frame ^[(param 0 counter)]

    thread 4 run_in_a_thread_lazy

    frame ^[(paref 0 4)]
    msg 0 detach
    free 4

    iinc counter
    jump loop
.mark: endloop

    print (strstore 4 "main/1 exited")

    izero 0
    end
.end
