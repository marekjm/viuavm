.function: send_me_your_pid/1
    allocate_registers %3 local

    move %1 local %0 parameters

    self %2 local
    send %1 local %2 local

    return
.end

.function: main/0
    allocate_registers %4 local

    self %1 local

    frame %1
    copy %0 arguments %1 local
    process void send_me_your_pid/1

    receive %2 local infinity

    pideq %3 local %1 local %1 local
    print %3 local

    pideq %3 local %2 local %1 local
    print %3 local

    izero %0 local
    return
.end
