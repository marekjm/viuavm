.import: [[dynamic]] std::os

.signature: std::os::lsdir/1
.signature: std::os::system/1

.function: print_entry/2
    allocate_registers %7 local

    .name: iota entry
    .name: iota path
    .name: iota status
    .name: iota key
    .name: iota control_sequence
    .name: iota highlight

    move %entry local %0 parameters
    move %highlight local %1 parameters

    atom %key local 'path'
    structat %path local %entry local %key local

    atom %key local 'is_directory'
    structat %status local %entry local %key local
    if *status local +1 is_a_regular_file
    text %status local "⇛ "
    jump print_path_part

    .mark: is_a_regular_file
    text %status local "  "
    
    .mark: print_path_part
    echo %status local

    if %highlight local +1 print_the_path
    string %control_sequence "\033[1m"
    echo %control_sequence local
    string %control_sequence "\033[4m"
    echo %control_sequence local

    .mark: print_the_path
    print *path local
    string %control_sequence "\033[0m"
    echo %control_sequence local

    return
.end
.function: print_entries/1
    allocate_registers %6 local

    .name: 5 highlight

    move %1 local %0 parameters

    integer %2 local 0
    vlen %3 local %1 local

    .mark: entry_printing_loop
    lt %4 local %2 local %3 local
    if %4 local +1 the_end

    vat %4 local %1 local %2 local
    frame %2
    copy %0 arguments *4 local
    not %highlight local %2 local
    move %1 arguments %highlight local
    call void print_entry/2

    iinc %2 local
    jump entry_printing_loop

    .mark: the_end
    return
.end

.function: tree_view_display_actor_impl/1
    allocate_registers %10 local

    .name: iota state
    .name: iota message
    .name: iota key
    .name: iota tag_shutdown
    .name: iota tag_data
    .name: iota got_tag
    .name: iota entries
    .name: iota tmp
    .name: iota control_sequence

    move %state local %0 parameters

    receive %message local infinity
    
    atom %tag_shutdown local 'shutdown'
    atom %tag_data local 'data'
    atom %key local 'tag'
    structat %got_tag local %message local %key local

    atomeq %tmp local *got_tag local %tag_shutdown local
    if %tmp local +1 check_tag_data
    text %tmp "tree_view_display_actor_impl shutting down"
    print %tmp local
    return

    .mark: check_tag_data
    atomeq %tmp local *got_tag local %tag_data local
    if %tmp local +1 the_end
    structremove %entries local %message local %tag_data local
    structinsert %state local %tag_data local %entries local

    .mark: check_tag_pointer_up

    .mark: printing_sequence
    string %control_sequence "\033[2J"
    echo %control_sequence local
    string %control_sequence "\033[1;1H"
    echo %control_sequence local

    structat %entries local %state local %tag_data local

    frame %1
    copy %0 arguments *entries local
    call void print_entries/1

    .mark: the_end
    frame %1
    move %0 arguments %state local
    tailcall tree_view_display_actor_impl/1
.end
.function: tree_view_display_actor/0
    allocate_registers %4 local

    .name: iota state
    .name: iota key
    .name: iota value

    struct %state local

    ; pointer is the index of item that should be highlighted
    atom %key local 'pointer'
    izero %value local 0
    structinsert %state local %key local %value local

    ; data is the list of items to display
    atom %key local 'data'
    vector %value local
    structinsert %state local %key local %value local

    frame %1
    move %0 arguments %state local
    tailcall tree_view_display_actor_impl/1
.end
.function: make_tagged_message_impl/1
    allocate_registers %4 local

    .name: iota message
    .name: iota tag
    .name: iota key

    move %tag local %0 parameters

    struct %message local
    atom %key local 'tag'
    structinsert %message local %key local %tag local

    move %0 local %message local
    return
.end
.function: make_data_message/1
    allocate_registers %5 local

    .name: iota message
    .name: iota tag
    .name: iota data
    .name: iota key

    atom %tag local 'data'
    frame %1
    move %0 arguments %tag local
    call %message local make_tagged_message_impl/1

    ; insert the data field: { tag: 'data', data: ... }
    atom %key local 'data'
    move %data local %0 parameters
    structinsert %message local %key local %data local

    move %0 local %message local
    return
.end
.function: make_shutdown_message/0
    allocate_registers %2 local

    .name: iota tag
    atom %tag local 'shutdown'

    frame %1
    move %0 arguments %tag local
    call %0 local make_tagged_message_impl/1
    return
.end
.function: make_pointer_up_message/0
    allocate_registers %2 local

    .name: iota tag
    atom %tag local 'pointer_up'

    frame %1
    move %0 arguments %tag local
    call %0 local make_tagged_message_impl/1
    return
.end
.function: make_pointer_down_message/0
    allocate_registers %2 local

    .name: iota tag
    atom %tag local 'pointer_down'

    frame %1
    move %0 arguments %tag local
    call %0 local make_tagged_message_impl/1
    return
.end

.function: time_to_shut_down/0
    allocate_registers %1 local

    integer %0 local 0
    not %0 local

    try
    catch "Exception" .block: catch_me
        draw void
        not %0 local
        leave
    .end
    enter .block: try_receiving
        receive void 0ms
        leave
    .end

    return
.end
.function: input_actor/1
    allocate_registers %8 local

    .name: iota tree_view_actor
    .name: iota tmp
    .name: iota stdin
    .name: iota buf
    .name: iota req

    .name: iota c_quit
    .name: iota c_refresh

    move %tree_view_actor local %0 parameters

    frame %0
    call %tmp local time_to_shut_down/0
    if %tmp local +1 await_input_stage
    text %tmp local "time to shut down"
    print %tmp local
    return

    .mark: await_input_stage
    integer %stdin local 0

    integer %buf local 1
    io_read %req local %stdin local %buf local
    io_wait %buf local %req local infinity

    print %buf local

    string %c_quit local "q"
    streq %c_quit local %buf local %c_quit local
    if %c_quit local the_end +1
    
    ;string %c_refresh local "r"
    ;streq %c_refresh local %buf local %c_refresh local
    ;if %c_refresh local refresh_display the_end

    ;.mark: refresh_display
    ;frame %1
    ;string %tmp local "./misc"
    ;copy %0 arguments %tmp local
    ;call %tmp local std::os::lsdir/1

    ;frame %1
    ;move %0 arguments %tmp local
    ;call %tmp local make_data_message/1
    ;send %tree_view_actor local %tmp local

    jump happy_loopin

    .mark: the_end
    frame %0
    call %tmp local make_shutdown_message/0
    send %tree_view_actor local %tmp local

    text %tmp "input_actor shutting down"
    print %tmp local
    return

    .mark: happy_loopin
    frame %1
    move %0 arguments %tree_view_actor local
    tailcall input_actor/1
.end

.function: return_tty_to_sanity/0
    allocate_registers %2 local

    .name: iota command

    string %command local "stty sane"
    frame %1
    move %0 arguments %command local
    call void std::os::system/1

    string %command local "stty cooked"
    frame %1
    move %0 arguments %command local
    call void std::os::system/1

    return
.end
.function: main/2
    allocate_registers %5 local

    .name: iota directory
    .name: iota tree_view_actor
    .name: iota input_actor
    .name: iota tmp

    frame %0
    defer return_tty_to_sanity/0

    move %directory local %1 parameters
    if %directory local +1 use_default_directory
    vpop %directory local %directory local void
    jump +2
    .mark: use_default_directory
    string %directory local "."

    print %directory local

    ;
    ; launch worker actors
    ;
    frame %0
    process %tree_view_actor local tree_view_display_actor/0

    frame %1
    copy %0 arguments %tree_view_actor local
    process %input_actor local input_actor/1
    ; process void input_actor/1

    ;
    ; set up initial directory listting
    ;
    frame %1
    copy %0 arguments %directory local
    call %tmp local std::os::lsdir/1
    frame %1
    move %0 arguments %tmp local
    call %tmp local make_data_message/1
    send %tree_view_actor local %tmp local

    ;
    ; join worker actors
    ;
    join void %tree_view_actor local
    join void %input_actor local

    izero %0 local
    return
.end
