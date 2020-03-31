.import: [[dynamic]] std::os

.signature: std::os::lsdir/1

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
    allocate_registers %8 local

    .name: iota message
    .name: iota key
    .name: iota tag_shutdown
    .name: iota tag_data
    .name: iota got_tag
    .name: iota tmp
    .name: iota control_sequence

    receive %message local infinity
    
    atom %tag_shutdown local 'shutdown'
    atom %tag_data local 'data'
    atom %key local 'tag'
    structat %got_tag local %message local %key local

    atomeq %tmp local *got_tag local %tag_shutdown local
    if %tmp local +1 check_tag_data
    return

    .mark: check_tag_data
    atomeq %tmp local *got_tag local %tag_data local
    if %tmp local +1 the_end
    structat %tmp local %message local %tag_data local

    string %control_sequence "\033[2J"
    echo %control_sequence local
    string %control_sequence "\033[1;1H"
    echo %control_sequence local

    frame %1
    copy %0 arguments *tmp local
    call void print_entries/1

    .mark: the_end
    frame %0
    tailcall tree_view_display_actor_impl/1
.end
.function: tree_view_display_actor/0
    allocate_registers %4 local

    .name: iota state
    .name: iota key
    .name: iota pointer_value

    struct %state local
    atom %key local 'pointer'
    izero %pointer_value local 0
    structinsert %state local %key local %pointer_value local

    frame %1
    move %0 arguments %state local
    tailcall tree_view_display_actor_impl/1
.end
.function: make_data_message/1
    allocate_registers %5 local

    .name: iota message
    .name: iota tag_data
    .name: iota data
    .name: iota key

    struct %message local

    ; insert the tag field: { tag: 'data' }
    atom %key local 'tag'
    atom %tag_data local 'data'
    structinsert %message local %key local %tag_data local

    ; insert the data field: { tag: 'data', data: ... }
    atom %key local 'data'
    move %data local %0 parameters
    structinsert %message local %key local %data local

    move %0 local %message local
    return
.end
.function: make_shutdown_message/0
    allocate_registers %4 local

    .name: iota message
    .name: iota tag_shutdown
    .name: iota key

    struct %message local

    ; insert the tag field: { tag: 'shutdown' }
    atom %key local 'tag'
    atom %tag_shutdown local 'shutdown'
    structinsert %message local %key local %tag_shutdown local

    move %0 local %message local
    return
.end

.function: main/2
    allocate_registers %4 local

    .name: iota directory
    .name: iota tree_view_actor
    .name: iota tmp

    move %directory local %1 parameters
    if %directory local +1 use_default_directory
    vpop %directory local %directory local void
    jump +2
    .mark: use_default_directory
    string %directory local "."

    print %directory local

    frame %0
    process %tree_view_actor local tree_view_display_actor/0

    frame %1
    copy %0 arguments %directory local
    call %tmp local std::os::lsdir/1
    frame %1
    move %0 arguments %tmp local
    call %tmp local make_data_message/1
    send %tree_view_actor local %tmp local

    ;integer %tmp local 1
    ;io_read %tmp local %tmp local %tmp local
    ;io_wait void %tmp local infinity

    frame %0
    call %tmp local make_shutdown_message/0
    send %tree_view_actor local %tmp local

    izero %0 local
    return
.end
