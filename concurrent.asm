.import: [[dynamic]] std::io

.signature: std::io::stdin::getline/0

.function: leaf/1
    allocate_registers %6 local

    .name: iota gate_guardian_pid
    move %gate_guardian_pid local %0 parameters

    .name: iota packet
    .name: iota sender
    .name: iota message
    .name: iota x
    struct %packet local

    atom %message local 'message'
    integer %x local 0
    structinsert %packet local %message local %x local

    atom %sender local 'sender'
    self %x local
    structinsert %packet local %sender local %x local

    send %gate_guardian_pid local %packet local
    receive %x local 2s
    print %x local

    return
.end

.function: grow/2
    allocate_registers %3 local

    .name: iota counter
    .name: iota gate_guardian_pid
    move %counter local %0 parameters
    move %gate_guardian_pid local %1 parameters

    idec %counter local
    if %counter local +1 no_more

    frame %3
    copy %0 arguments %counter local
    copy %1 arguments %gate_guardian_pid local
    move %2 arguments %counter local
    process void grow_more/3

    .mark: no_more
    frame %1
    move %0 arguments %gate_guardian_pid local
    tailcall leaf/1
.end

.function: grow_more/3
    allocate_registers %4 local

    .name: iota counter
    .name: iota gate_guardian_pid
    .name: iota n
    move %counter local %0 parameters
    move %gate_guardian_pid local %1 parameters
    move %n local %2 parameters

    if %n local +1 this_is_the_end

    frame %2
    copy %0 arguments %counter local
    copy %1 arguments %gate_guardian_pid local
    process void grow/2

    idec %n local
    frame %3
    move %0 arguments %counter local
    move %1 arguments %gate_guardian_pid local
    move %2 arguments %n local
    tailcall grow_more/3

    .mark: this_is_the_end
    return
.end

.function: gate_guardian_impl/1
    allocate_registers %7 local

    .name: iota counter
    move %counter local %0 parameters

    .name: iota packet
    .name: iota message
    receive %packet local
    ;print %packet local

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

    text %0 local "awaiting gate_guardian's PID"
    print %0 local

    .name: iota gate_guardian_pid
    receive %gate_guardian_pid local 1s

    text %0 local "got gate_guardian's PID"
    print %0 local

    frame %2
    move %0 arguments %no_of_processes local
    copy %1 arguments %gate_guardian_pid local
    call grow/2

    frame %0
    call void std::io::stdin::getline/0
    text %0 local "sending kill signal"
    print %0 local

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
