.function: hey_im_a_thread
    print (strstore 1 "Hey, I'm a thread!")
    end
.end

.function: thread_lanucher
    frame 0
    thread 1 hey_im_a_thread
    ;frame ^[(param 0 1)]
    ;msg 0 detach
    thjoin 1
    end
.end

.function: main
    frame 0
    thread 1 thread_lanucher
    frame ^[(param 0 1)]
    msg 0 detach

    izero 0
    end
.end
