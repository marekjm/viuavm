.function: run_in_a_thread
    print (threceive 1)
    end
.end

.function: main
    frame 0
    thread 1 run_in_a_thread

    frame ^[(param 0 1)]
    msg 0 detach

    frame ^[(param 0 1) (param 1 (istore 2 40))]
    msg 0 setPriority

    frame ^[(param 0 1) (param 1 (strstore 2 "Hello message passing World!"))]
    msg 0 pass

    izero 0
    end
.end
