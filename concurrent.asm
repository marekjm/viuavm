.function: grow/2
    allocate_registers %3 local

    .name: iota counter
    .name: iota gate_guardian_pid
    move %counter local %0 parameters
    move %gate_guardian_pid local %1 parameters

    print %counter local
    print %gate_guardian_pid local

    return
.end

.function: gate_guardian_impl/1
    allocate_registers %7 local

    .name: iota counter
    move %counter local %0 parameters

    .name: iota packet
    .name: iota message
    receive %packet local
    print %packet local

    atom %message local 'message'
    structat %message local %packet local %message local

    .name: iota terminator_message
    integer %terminator_message local -42

    eq %terminator_message local %terminator_message local *message local 
    if %terminator_message local +1 reply_and_continue
    return

    .mark: reply_and_continue

    .name: iota sender
    atom %sender local 'sender'
    structat %sender local %packet local %sender local

    .name: iota n
    copy %n local %counter local
    send *sender local %n local

    iinc %counter local

    frame %1
    move %0 arguments %counter local
    tailcall gate_guardian_impl/1
.end
.function: gate_guardian/1
    allocate_registers %3 local

    .name: iota parent
    .name: iota self_pid

    move %parent local %0 parameters
    self %self_pid local
    send %parent local %self_pid local

    frame %1
    izero %0 local
    move %0 arguments %0 local
    tailcall gate_guardian_impl/1
.end

.function: main/2
    allocate_registers %7 local

    ; Make arguments accessible.
    .name: iota argv
    move %argv local %1 parameters

    ; Determine the number of processes to grow.
    .name: iota no_of_processes
    izero %no_of_processes local
    vat %no_of_processes local %argv local %no_of_processes local
    stoi %no_of_processes local *no_of_processes local

    frame %1
    self %0 local
    move %0 arguments %0 local
    process void gate_guardian/1

    .name: iota gate_guardian_pid
    receive %gate_guardian_pid local 1s

    frame %2
    move %0 arguments %no_of_processes local
    copy %1 arguments %gate_guardian_pid local
    call grow/2

    .name: iota packet
    .name: iota message
    .name: iota n
    struct %packet local
    atom %message local 'message'
    integer %n local -42
    structinsert %packet local %message local %n local
    send %gate_guardian_pid local %packet local

    izero %0 local
    return
.end
